#include "distributed/dataserver.h"
#include "common/util.h"

namespace chfs {

auto DataServer::initialize(std::string const &data_path) {
  /**
   * At first check whether the file exists or not.
   * If so, which means the distributed chfs has
   * already been initialized and can be rebuilt from
   * existing data.
   */
  bool is_initialized = is_file_exist(data_path);

  auto bm = std::shared_ptr<BlockManager>(
      new BlockManager(data_path, KDefaultBlockCnt));
  if (is_initialized) {
    block_allocator_ = std::make_shared<BlockAllocator>(bm, 0, false);
  } else {
    // We need to reserve some blocks for storing the version of each block
    block_allocator_ =
        std::shared_ptr<BlockAllocator>(new BlockAllocator(bm, 0, true));
  }

  // Initialize the RPC server and bind all handlers
  server_->bind("read_data", [this](block_id_t block_id, usize offset,
                                    usize len, version_t version) {
    return this->read_data(block_id, offset, len, version);
  });
  server_->bind("write_data", [this](block_id_t block_id, usize offset,
                                     std::vector<u8> &buffer) {
    return this->write_data(block_id, offset, buffer);
  });
  server_->bind("alloc_block", [this]() { return this->alloc_block(); });
  server_->bind("free_block", [this](block_id_t block_id) {
    return this->free_block(block_id);
  });

  // Launch the rpc server to listen for requests
  server_->run(true, num_worker_threads);
}

DataServer::DataServer(u16 port, const std::string &data_path)
    : server_(std::make_unique<RpcServer>(port)) {
  initialize(data_path);
}

DataServer::DataServer(std::string const &address, u16 port,
                       const std::string &data_path)
    : server_(std::make_unique<RpcServer>(address, port)) {
  initialize(data_path);
}

DataServer::~DataServer() { server_.reset(); }

// FIXME: WHAT IS THE VERSION???
// {Your code here}
auto DataServer::read_data(block_id_t block_id, usize offset, usize len,
                           version_t version) -> std::vector<u8> {
  // TODO: Implement this function.
  usize block_size = this->block_allocator_->bm->block_size();
  if (offset + len - 1 > block_size - 1) {
    std::vector<u8> zero_data(0);
    return zero_data;
  }
  std::vector<u8> buffer(block_size);
  ChfsNullResult result =
      this->block_allocator_->bm->read_block(block_id, buffer.data());
  if (result.is_ok()) {
    std::vector<u8> return_data(len);
    auto start = buffer.begin() + offset;
    auto end = buffer.begin() + offset + len;
    std::copy(start, end, return_data.begin());
    return return_data;
  } else {
    std::vector<u8> zero_data(0);
    return zero_data;
  }
}

// {Your code here}
auto DataServer::write_data(block_id_t block_id, usize offset,
                            std::vector<u8> &buffer) -> bool {
  // TODO: Implement this function.
  usize block_size = this->block_allocator_->bm->block_size();
  usize len = buffer.size();
  if (offset + len - 1 > block_size - 1)
    return false;
  ChfsNullResult result = this->block_allocator_->bm->write_partial_block(
      block_id, buffer.data(), offset, len);
  if (result.is_ok())
    return true;
  else
    return false;
}

// FIXME: WHERE IS THE VERSION???
//  {Your code here}
auto DataServer::alloc_block() -> std::pair<block_id_t, version_t> {
  // TODO: Implement this function.
  ChfsResult<block_id_t> result = this->block_allocator_->allocate();
  if (result.is_ok()) {
    std::pair<block_id_t, version_t> return_value =
        std::make_pair<block_id_t, version_t>(result.unwrap(), 1);
    return return_value;
  } else {
    // FIXME: what should I do if there is an error?
    std::pair<block_id_t, version_t> return_value =
        std::make_pair<block_id_t, version_t>(0, 0);
    return return_value;
  }
}

// {Your code here}
auto DataServer::free_block(block_id_t block_id) -> bool {
  // TODO: Implement this function.
  ChfsNullResult result = this->block_allocator_->deallocate(block_id);
  if (result.is_ok())
    return true;
  else
    return false;
}
} // namespace chfs