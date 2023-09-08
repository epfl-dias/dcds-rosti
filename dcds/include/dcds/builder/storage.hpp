//
// Created by prathamesh on 11/8/23.
//

#ifndef DCDS_STORAGE_HPP
#define DCDS_STORAGE_HPP

#include <dcds/builder/attribute.hpp>
#include <dcds/storage/table-registry.hpp>

using DSValueType = std::variant<int64_t, void *>;

namespace dcds {
class StorageLayer {
 public:
  explicit StorageLayer(const std::string &txn_namespace,
                        std::map<std::string, std::shared_ptr<dcds::Attribute>> attributes_) {
    this->attributes = attributes_;
    this->init(txn_namespace);
  }

  void read(void *readVariable, uint64_t attributeIndex, void *txnPtr);
  void write(void *writeVariable, uint64_t attributeIndex, void *txnPtr);

  void *beginTxn();
  void commitTxn(void *txn);

 private:
  std::shared_ptr<dcds::txn::TransactionManager> txnManager;
  dcds::storage::record_reference_t mainRecord;
  std::shared_ptr<dcds::storage::Table> dsTable;
  std::map<std::string, std::shared_ptr<dcds::Attribute>> attributes;

  void init(const std::string &txn_namespace);
  void initTables(const std::string &txnNamespace, const std::string &dsTableName);
};
}  // namespace dcds

#endif  // DCDS_STORAGE_HPP
