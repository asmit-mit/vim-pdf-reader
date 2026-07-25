#include <stdexcept>

#include "core/render_scheduler.h"
#include "utils/settings.h"

namespace core {

RenderScheduler::RenderScheduler(
    pdf::PDFDocument &document, pdf::PDFRenderer &renderer, std::size_t worker_count
)
    : document_(document), renderer_(renderer), texture_cache_(settings::cache_size_),
      image_cache_(settings::cache_size_), main_ctx_(document_.cloneContext()), active_jobs_(0),
      quiescing_(false), stop_(false) {
  worker_context_.reserve(worker_count);
  for (std::size_t i = 0; i < worker_count; i++)
    worker_context_.push_back(document_.cloneContext());

  workers_.reserve(worker_count);
  for (std::size_t i = 0; i < worker_count; i++)
    workers_.emplace_back(&RenderScheduler::workerLoop, this, i);
}

RenderScheduler::~RenderScheduler() {
  {
    std::lock_guard lock(lock_);
    clearCacheLocked();
    stop_ = true;
  }

  cv_work_.notify_all();
  for (auto &worker : workers_)
    worker.join();
}

void RenderScheduler::request(const pdf::PDFRenderKey &key) {
  {
    std::lock_guard lock(lock_);

    if (quiescing_)
      return;

    if (key.page_idx >= document_.size())
      throw std::out_of_range("Page index out of range");

    if (texture_cache_.contains(key))
      return;

    if (image_cache_.contains(key))
      return;

    if (!pending_.insert(key).second)
      return;

    if (job_queue_.size() >= max_queue_size_) {
      pending_.erase(job_queue_.front());
      job_queue_.pop_front();
    }

    job_queue_.push_back(key);
  }

  cv_work_.notify_one();
}

const sf::Vector2u RenderScheduler::getPageSize(const pdf::PDFRenderKey &key) {
  std::lock_guard lock(lock_);

  if (quiescing_)
    throw std::runtime_error("Scheduler is quiescing; no document is currently valid");

  if (key.page_idx >= document_.size())
    throw std::out_of_range("Page index out of range");

  return renderer_.getPageSize(main_ctx_.get(), document_.getDoc(), key);
}

bool RenderScheduler::isReady(const pdf::PDFRenderKey &key) {
  std::lock_guard lock(lock_);
  return texture_cache_.contains(key) || image_cache_.contains(key);
}

sf::Texture *RenderScheduler::getTexture(const pdf::PDFRenderKey &key) {
  std::lock_guard lock(lock_);

  if (auto *texture = texture_cache_.get(key))
    return texture;

  auto *image = image_cache_.get(key);
  if (!image)
    return nullptr;

  sf::Texture texture;
  if (!texture.loadFromImage(*image))
    throw std::runtime_error("Failed to load image into texture");

  texture_cache_.put(key, std::move(texture));
  return texture_cache_.get(key);
}

void RenderScheduler::clearCache() {
  std::lock_guard lock(lock_);
  clearCacheLocked();
}

void RenderScheduler::quiesce() {
  std::unique_lock lock(lock_);
  quiescing_ = true;

  job_queue_.clear();
  pending_.clear();

  cv_idle_.wait(lock, [&] { return active_jobs_ == 0; });
}

void RenderScheduler::resume() {
  std::lock_guard lock(lock_);
  quiescing_ = false;
}

void RenderScheduler::workerLoop(std::size_t idx) {
  while (true) {
    pdf::PDFRenderKey key;

    std::unique_lock lock(lock_);
    cv_work_.wait(lock, [&] { return stop_ || !job_queue_.empty(); });

    if (stop_ && job_queue_.empty())
      return;

    key = job_queue_.back();
    job_queue_.pop_back();
    active_jobs_++;
    lock.unlock();

    sf::Image image = renderer_.render(worker_context_[idx].get(), document_.getDoc(), key);

    lock.lock();
    image_cache_.put(key, std::move(image));
    pending_.erase(key);
    active_jobs_--;
    if (active_jobs_ == 0)
      cv_idle_.notify_all();
  }
}

void RenderScheduler::clearCacheLocked() {
  texture_cache_.clear();
  image_cache_.clear();
  pending_.clear();
  job_queue_.clear();

  renderer_.clearCache();
}

} // namespace core
