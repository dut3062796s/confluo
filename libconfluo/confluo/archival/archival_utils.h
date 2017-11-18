#ifndef CONFLUO_ARCHIVAL_UTILS_H_
#define CONFLUO_ARCHIVAL_UTILS_H_

#include "incr_file_writer.h"
#include "container/reflog.h"
#include "encoder.h"

namespace confluo {
namespace archival {

using namespace ::utils;
using namespace ::confluo::storage;

class archival_utils {
 public:
  typedef storage::read_only_ptr<uint64_t> bucket_ptr_t;
  typedef bucket_ptr_t::decoded_ptr decoded_ptr_t;
  typedef encoder::raw_encoded_ptr raw_encoded_ptr_t;
  typedef encoded_ptr<uint64_t> encoded_ptr_t;

  /**
   * Archives a reflog belonging to a filter.
   * @param reflog aggregated reflog
   * @param writer writer
   * @param offset data log offset to archive up until
   * @param archival_tail reflog offset to start archival from
   * @return reflog offset archived up to
   */
  template<encoding_type ENCODING>
  static size_t archive_reflog(reflog& reflog, incremental_file_writer& writer, size_t offset,
                               size_t archival_tail = 0) {
    bucket_ptr_t bucket_ptr;
    size_t data_log_archival_tail = 0;
    // TODO replace w/ iterator
    while (data_log_archival_tail < offset && archival_tail < reflog.size()) {

      reflog.ptr(archival_tail, bucket_ptr);
      decoded_ptr_t decoded_ptr = bucket_ptr.decode_ptr();
      auto* metadata = ptr_metadata::get(bucket_ptr.get().internal_ptr());
      uint64_t* data = decoded_ptr.get();

      if (metadata->state_ != state_type::D_IN_MEMORY) {
        archival_tail += reflog_constants::BUCKET_SIZE;
        continue;
      }

      data_log_archival_tail = max_in_bucket(data);
      if (data_log_archival_tail >= offset) {
        break;
      }

      size_t encoded_size;
      raw_encoded_ptr_t unbacked_encoded_bucket = encoder::encode<uint64_t, ENCODING>(data, encoded_size);
      size_t off = writer.append<ptr_metadata, uint8_t>(metadata, 1, unbacked_encoded_bucket.get(),
                                                        encoded_size);
      writer.update_header(data_log_archival_tail);
      void* encoded_bucket = ALLOCATOR.mmap(writer.cur_path(), off, encoded_size, state_type::D_ARCHIVED);
      reflog.swap_bucket_ptr(archival_tail, encoded_ptr_t(encoded_bucket));
      archival_tail += reflog_constants::BUCKET_SIZE;
    }
    return archival_tail;
  }

 private:
  static uint64_t max_in_bucket(uint64_t* data) {
    uint64_t max = 0;
    for (size_t i = 0; i < reflog_constants::BUCKET_SIZE; i++) {
      // Account for memset in last bucket of a reflog
      if (data[i] == (uint64_t) -1) {
        break;
      }
      max = std::max(max, data[i]);
    }
    return max;
  }

};

}
}

#endif /* CONFLUO_ARCHIVAL_UTILS_H_ */
