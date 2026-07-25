#pragma once

#include "pdf/pdf_document.h"
#include "pdf/pdf_renderer.h"
#include "utils/lru_cache.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

namespace core {

using TextureCache = utils::LRUCache<pdf::PDFRenderKey, sf::Texture, pdf::PDFRenderKeyHash>;
using ImageCache = utils::LRUCache<pdf::PDFRenderKey, sf::Image, pdf::PDFRenderKeyHash>;

class RenderScheduler {
public:
  RenderScheduler(pdf::PDFDocument &document, pdf::PDFRenderer &renderer, std::size_t worker_count);
  ~RenderScheduler();

  void request(const pdf::PDFRenderKey &key);
  bool isReady(const pdf::PDFRenderKey &key);
  sf::Texture *getTexture(const pdf::PDFRenderKey &key);
  const sf::Vector2u getPageSize(const pdf::PDFRenderKey &key);
  void clearCache();

  void quiesce();
  void resume();

private:
  void workerLoop(std::size_t idx);
  void clearCacheLocked();

private:
  pdf::PDFDocument &document_;
  pdf::PDFRenderer &renderer_;
  std::vector<std::thread> workers_;
  std::vector<pdf::FzContextPtr> worker_context_;

  TextureCache texture_cache_;
  ImageCache image_cache_;
  std::unordered_set<pdf::PDFRenderKey, pdf::PDFRenderKeyHash> pending_;

  std::deque<pdf::PDFRenderKey> job_queue_;

  std::mutex lock_;
  std::condition_variable cv_work_;
  std::condition_variable cv_idle_;

  pdf::FzContextPtr main_ctx_;

  std::size_t active_jobs_;
  bool quiescing_;
  bool stop_;

  static constexpr std::size_t max_queue_size_ = 8;
};

} // namespace core
