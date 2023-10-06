//
// Created by prathamesh on 11/8/23.
//

#ifndef DCDS_STORAGE_HPP
#define DCDS_STORAGE_HPP

#include <dcds/builder/attribute.hpp>
#include <dcds/storage/table-registry.hpp>

using DSValueType = std::variant<int64_t, void *>;

namespace dcds {

/// Class to represent storage layer in DCDS
class StorageLayer {
 public:
  ///
  /// \param txn_namespace transaction namespace for transactions
  /// \param attributes_   Data structure attributes for the storage table
  explicit StorageLayer(const std::string &txn_namespace,
                        std::map<std::string, std::shared_ptr<dcds::Attribute>> attributes_) {
    this->attributes = attributes_;
    this->init(txn_namespace);
  }

  ///
  /// \param readVariable     Record reference pointer for the set of attributes to be read
  /// \param attributeIndex   Index of attribute in the set of attributes to be read
  /// \param txnPtr           transaction pointer for the transactions
  void read(void *readVariable, uint64_t attributeIndex, void *txnPtr);
  ///
  /// \param writeVariable    Record reference pointer for the set of attributes to be written
  /// \param attributeIndex   Index of attribute in the set of attributes to be written
  /// \param txnPtr           transaction pointer for the transactions
  void write(void *writeVariable, uint64_t attributeIndex, void *txnPtr);

  ///
  /// \return trasaction pointer for the transactions
  void *beginTxn();
  ///
  /// \param txn transaction pointer for the transactions
  void commitTxn(void *txn);

 private:
  /// transaction manager for the transactions
  std::shared_ptr<dcds::txn::TransactionManager> txnManager;
  /// main record for the storage
  dcds::storage::record_reference_t mainRecord;
  /// Table storing data structure attributes
  std::shared_ptr<dcds::storage::Table> dsTable;
  /// Map attribute names with attributes
  std::map<std::string, std::shared_ptr<dcds::Attribute>> attributes;

  ///
  /// \param txnNamespace transaction namespace for the transactions
  void init(const std::string &txnNamespace);
  ///
  /// \param txnNamespace transaction namespace for the transactions
  /// \param dsTableName  Name of the data structure in storage layer
  void initTables(const std::string &txnNamespace, const std::string &dsTableName);
};
}  // namespace dcds

#endif  // DCDS_STORAGE_HPP
