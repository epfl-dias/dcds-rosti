/*
                              Copyright (c) 2023.
          Data Intensive Applications and Systems Laboratory (DIAS)
                  École Polytechnique Fédérale de Lausanne

                              All Rights Reserved.

      Permission to use, copy, modify and distribute this software and
      its documentation is hereby granted, provided that both the
      copyright notice and this permission notice appear in all copies of
      the software, derivative works or modified versions, and any
      portions thereof, and that both notices appear in supporting
      documentation.

      This code is distributed in the hope that it will be useful, but
      WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
      DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
      RESULTING FROM THE USE OF THIS SOFTWARE.
 */

#ifndef DCDS_ART_KEY_HPP
#define DCDS_ART_KEY_HPP

#include <iostream>

namespace dcds::indexes {

using key_unit_t = uint8_t;
using len_t = size_t;

template <size_t N>
class ARTKeyFixedWidth {
 public:
  explicit ARTKeyFixedWidth(key_unit_t *data) {
    memcpy(_data, data, N);
    /*
    switch (N) {
      case 8:
        *(reinterpret_cast<uint64_t*>(_data)) = *(reinterpret_cast<uint64_t*>(data));
        break;
      case 4:
        *(reinterpret_cast<uint32_t*>(_data)) = *(reinterpret_cast<uint32_t*>(data));
        break;
      case 2:
        *(reinterpret_cast<uint16_t*>(_data)) = *(reinterpret_cast<uint16_t*>(data));
        break;
      case 1:
        *(reinterpret_cast<uint8_t*>(_data)) = *(reinterpret_cast<uint8_t*>(data));
        break;
      default:
        memcpy(_data, data, N);
    }*/
  }

  key_unit_t &operator[](auto i) { return _data[i]; }
  const key_unit_t &operator[](auto i) const { return _data[i]; }

 private:
  key_unit_t _data[N];
};

class ARTKeyVariableWidth {
 public:
  ARTKeyVariableWidth(key_unit_t *key_data, len_t key_len) {
    _data = static_cast<key_unit_t *>(malloc(key_len));
    memcpy(_data, key_data, key_len);
  }

  key_unit_t &operator[](std::size_t i) { return _data[i]; }
  const key_unit_t &operator[](std::size_t i) const { return _data[i]; }

 private:
  key_unit_t *_data;
};

template <typename K>
class ARTKey {
 public:
  len_t _key_len;
  typename std::conditional<std::is_integral<K>::value, ARTKeyFixedWidth<sizeof(K)>, ARTKeyVariableWidth>::type
      _key_holder;

  explicit ARTKey(const key_unit_t *data, len_t key_len) : _key_len(key_len), _key_holder(data) {}

  ARTKey(ARTKey const &artKey) : _key_len(artKey._key_len), _key_holder(artKey._key_holder) {}
  ARTKey(ARTKey const &&artKey) noexcept
      : _key_len(std::move(artKey._key_len)), _key_holder(std::move(artKey._key_holder)) {}

  ARTKey &operator=(ARTKey const &artKey) {
    _key_len = artKey._key_len;
    _key_holder = artKey._key_holder;
    return *this;
  }

  ARTKey &operator=(ARTKey &&artKey) noexcept {
    _key_len = std::move(artKey._key_len);
    _key_holder = std::move(artKey._key_holder);
    return *this;
  }

  key_unit_t *getData() { return _key_holder.data(); }

  static void destroy(ARTKey *k) {}
};

// template <typename T, size_t N = sizeof(T)>
// class BinaryComparable{
//
//   using key_holder_t = std::conditional<std::is_integral<T>::value,
//                                         ARTKeyFixedWidth<sizeof(T)>,
//                                         ARTKeyVariableWidth>::type;
//
//   const bool is_fixed_width = std::is_integral<T>::value;
//
//   BinaryComparable(key_unit_t *data, len_t key_len): {
//     if(std::endian::native == std::endian::little){
//       // convert
//     }
//
//   }
//
//
//
//  private:
//   inline void setByteOrder(key_unit_t *__restrict newkey, K &k) {
////    if (std::endian::native == std::endian::little) {
////      convertKey(newkey, key);
////    }
//
//    if (typeid(K) != typeid(std::string)) return;
//    switch (sizeof(K)) {
//      case 16:
//        *((K *)newkey) = __builtin_bswap16(k);
//        break;
//      case 32:
//        *((K *)newkey) = __builtin_bswap32(k);
//        break;
//      case 64:
//        *((K *)newkey) = __builtin_bswap64(k);
//        break;
//      default:
//        std::reverse(newkey, newkey + sizeof(k));
//    }
//  }
//
//
//  key_holder_t key_data;
//
//};

}  // namespace dcds::indexes

#endif  // DCDS_ART_KEY_HPP
