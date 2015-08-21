// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "syzygy/refinery/types/typed_data.h"

#include <stddef.h>
#include <stdint.h>

#include "gtest/gtest.h"
#include "syzygy/refinery/core/bit_source.h"
#include "syzygy/refinery/types/type.h"
#include "syzygy/refinery/types/type_repository.h"

namespace refinery {

namespace {

struct TestUDT {
  uint16_t one;
  struct InnerUDT {
    uint8_t inner_one;
    uint32_t inner_two;
  } two;
  const TestUDT* three;
  int32_t four : 10;
  int32_t five : 10;
  int32_t six[10];
};

const TestUDT test_instance =
    {1, {2, 3}, &test_instance, 4, -5, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}};

// A bit source that reflects the process' own memory.
class TestBitSource : public BitSource {
 public:
  ~TestBitSource() override {}

  bool GetAll(const AddressRange& range, void* data_ptr) override {
    ::memcpy(data_ptr, reinterpret_cast<void*>(range.addr()), range.size());
    return true;
  }

  bool GetFrom(const AddressRange& range,
               size_t* data_cnt,
               void* data_ptr) override {
    *data_cnt = range.size();
    return GetAll(range, data_ptr);
  }

  bool HasSome(const AddressRange& range) override { return true; }
};

class TypedDataTest : public testing::Test {
 protected:
  void SetUp() override {
    udt_ = CreateUDTType();
    ASSERT_TRUE(udt_);
  }

  TypedData GetTestInstance() {
    return TypedData(
        test_bit_source(), udt_,
        AddressRange(ToAddr(&test_instance), sizeof(test_instance)));
  }

  Address ToAddr(const void* ptr) { return reinterpret_cast<Address>(ptr); }

  void AssertFieldMatchesDataType(const TestUDT* ignore,
                                  const TypedData& data) {
    ASSERT_TRUE(data.IsPrimitiveType());
    ASSERT_TRUE(data.IsPointerType());
  }
  void AssertFieldMatchesDataType(const TestUDT::InnerUDT& ignore,
                                  const TypedData& data) {
    ASSERT_FALSE(data.IsPrimitiveType());
    ASSERT_FALSE(data.IsPointerType());
  }
  void AssertFieldMatchesDataType(uint8_t ignore, const TypedData& data) {
    ASSERT_TRUE(data.IsPrimitiveType());
    ASSERT_FALSE(data.IsPointerType());
  }
  void AssertFieldMatchesDataType(uint16_t ignore, const TypedData& data) {
    ASSERT_TRUE(data.IsPrimitiveType());
    ASSERT_FALSE(data.IsPointerType());
  }
  void AssertFieldMatchesDataType(uint32_t ignore, const TypedData& data) {
    ASSERT_TRUE(data.IsPrimitiveType());
    ASSERT_FALSE(data.IsPointerType());
  }

  template <typename FieldType>
  void AssertFieldMatchesData(const FieldType& field, const TypedData& data) {
    AssertFieldMatchesDataType(field, data);
    ASSERT_EQ(ToAddr(&field), data.range().addr());
    ASSERT_EQ(sizeof(field), data.range().size());
  }

  UserDefinedTypePtr udt() const { return udt_; }

 private:
  UserDefinedTypePtr CreateUDTType() {
    TypePtr uint8_type = new BasicType(L"uint8_t", sizeof(uint8_t));
    TypePtr uint16_type = new BasicType(L"uint16_t", sizeof(uint16_t));
    TypePtr uint32_type = new BasicType(L"uint32_t", sizeof(uint32_t));
    TypePtr int32_type = new BasicType(L"int32_t", sizeof(int32_t));
    repo_.AddType(uint8_type);
    repo_.AddType(uint16_type);
    repo_.AddType(uint32_type);
    repo_.AddType(int32_type);

    UserDefinedType::Fields fields;
    UserDefinedTypePtr inner(
        new UserDefinedType(L"Inner", sizeof(TestUDT::InnerUDT)));
    fields.push_back(UserDefinedType::Field(
        L"inner_one", offsetof(TestUDT::InnerUDT, inner_one), 0, 0, 0,
        uint8_type->type_id()));
    fields.push_back(UserDefinedType::Field(
        L"inner_two", offsetof(TestUDT::InnerUDT, inner_two), 0, 0, 0,
        uint32_type->type_id()));
    inner->Finalize(fields);
    repo_.AddType(inner);

    fields.clear();
    UserDefinedTypePtr outer(new UserDefinedType(L"TestUDT", sizeof(TestUDT)));
    PointerTypePtr ptr_type(
        new PointerType(sizeof(TestUDT*), PointerType::PTR_MODE_PTR));
    repo_.AddType(ptr_type);

    fields.push_back(UserDefinedType::Field(L"one", offsetof(TestUDT, one), 0,
                                            0, 0, uint16_type->type_id()));
    fields.push_back(UserDefinedType::Field(L"two", offsetof(TestUDT, two), 0,
                                            0, 0, inner->type_id()));
    fields.push_back(UserDefinedType::Field(L"three", offsetof(TestUDT, three),
                                            0, 0, 0, ptr_type->type_id()));
    fields.push_back(UserDefinedType::Field(
        L"four", offsetof(TestUDT, three) + sizeof(test_instance.three), 0, 0,
        10, int32_type->type_id()));
    fields.push_back(UserDefinedType::Field(
        L"five", offsetof(TestUDT, three) + sizeof(test_instance.three), 0, 10,
        10, int32_type->type_id()));

    ArrayTypePtr array_type = new ArrayType(sizeof(test_instance.six));
    array_type->Finalize(kNoTypeFlags, uint32_type->type_id(),
                         arraysize(test_instance.six), int32_type->type_id());
    repo_.AddType(array_type);

    fields.push_back(UserDefinedType::Field(L"six", offsetof(TestUDT, six),
                                            kNoTypeFlags, 0, 0,
                                            array_type->type_id()));
    outer->Finalize(fields);
    repo_.AddType(outer);

    ptr_type->Finalize(Type::FLAG_CONST, outer->type_id());

    return outer;
  }

  BitSource* test_bit_source() { return &test_bit_source_; }

  UserDefinedTypePtr udt_;
  TestBitSource test_bit_source_;
  TypeRepository repo_;
};

}  // namespace

TEST_F(TypedDataTest, GetNamedField) {
  TypedData data(GetTestInstance());

  TypedData one;
  ASSERT_TRUE(data.GetNamedField(L"one", &one));
  AssertFieldMatchesData(test_instance.one, one);

  TypedData two;
  ASSERT_TRUE(data.GetNamedField(L"two", &two));
  AssertFieldMatchesData(test_instance.two, two);

  TypedData inner_one;
  ASSERT_TRUE(two.GetNamedField(L"inner_one", &inner_one));
  AssertFieldMatchesData(test_instance.two.inner_one, inner_one);

  TypedData inner_two;
  ASSERT_TRUE(two.GetNamedField(L"inner_two", &inner_two));
  AssertFieldMatchesData(test_instance.two.inner_two, inner_two);

  TypedData three;
  ASSERT_TRUE(data.GetNamedField(L"three", &three));
  AssertFieldMatchesData(test_instance.three, three);

  TypedData four;
  ASSERT_TRUE(data.GetNamedField(L"four", &four));
  ASSERT_EQ(0, four.bit_pos());
  ASSERT_EQ(10, four.bit_len());

  TypedData five;
  ASSERT_TRUE(data.GetNamedField(L"five", &five));
  ASSERT_EQ(10, five.bit_pos());
  ASSERT_EQ(10, five.bit_len());

  TypedData six;
  ASSERT_TRUE(data.GetNamedField(L"six", &six));
  ASSERT_TRUE(six.IsArrayType());
}

TEST_F(TypedDataTest, GetField) {
  TypedData data(GetTestInstance());
  const UserDefinedType::Fields& data_fields = udt()->fields();

  TypedData one;
  ASSERT_TRUE(data.GetField(data_fields[0], &one));
  AssertFieldMatchesData(test_instance.one, one);

  TypedData two;
  ASSERT_TRUE(data.GetField(data_fields[1], &two));
  AssertFieldMatchesData(test_instance.two, two);

  UserDefinedTypePtr inner_udt;
  ASSERT_TRUE(two.type()->CastTo(&inner_udt));
  const UserDefinedType::Fields& inner_fields = inner_udt->fields();

  TypedData inner_one;
  ASSERT_TRUE(two.GetField(inner_fields[0], &inner_one));
  AssertFieldMatchesData(test_instance.two.inner_one, inner_one);

  TypedData inner_two;
  ASSERT_TRUE(two.GetField(inner_fields[1], &inner_two));
  AssertFieldMatchesData(test_instance.two.inner_two, inner_two);

  TypedData three;
  ASSERT_TRUE(data.GetField(data_fields[2], &three));
  AssertFieldMatchesData(test_instance.three, three);
}

TEST_F(TypedDataTest, GetSignedValue) {
  TypedData data(GetTestInstance());

  TypedData four;
  ASSERT_TRUE(data.GetNamedField(L"four", &four));
  int64_t value = 0;
  ASSERT_TRUE(four.GetSignedValue(&value));
  EXPECT_EQ(4, value);

  TypedData five;
  ASSERT_TRUE(data.GetNamedField(L"five", &five));
  ASSERT_TRUE(five.GetSignedValue(&value));
  EXPECT_EQ(-5, value);
}

TEST_F(TypedDataTest, GetUnsignedValue) {
  TypedData data(GetTestInstance());

  TypedData four;
  ASSERT_TRUE(data.GetNamedField(L"four", &four));
  uint64_t value = 0;
  ASSERT_TRUE(four.GetUnsignedValue(&value));
  EXPECT_EQ(4, value);

  TypedData five;
  ASSERT_TRUE(data.GetNamedField(L"five", &five));
  ASSERT_TRUE(five.GetUnsignedValue(&value));
  EXPECT_EQ(0x3FB, value);
}

TEST_F(TypedDataTest, GetPointerValue) {
  TypedData data(GetTestInstance());

  TypedData three;
  ASSERT_TRUE(data.GetNamedField(L"three", &three));
  Address addr = 0;
  ASSERT_TRUE(three.GetPointerValue(&addr));

  EXPECT_EQ(reinterpret_cast<uintptr_t>(test_instance.three), addr);
}

TEST_F(TypedDataTest, Dereference) {
  TypedData data(GetTestInstance());

  TypedData three;
  ASSERT_TRUE(data.GetNamedField(L"three", &three));

  TypedData derefenced;
  ASSERT_TRUE(three.Dereference(&derefenced));

  // Make sure the dereferenced object is identical.
  ASSERT_TRUE(derefenced.bit_source() == data.bit_source());
  ASSERT_TRUE(derefenced.type() == data.type());
  ASSERT_TRUE(derefenced.range() == data.range());
}

TEST_F(TypedDataTest, GetArrayElement) {
  TypedData data(GetTestInstance());

  TypedData array;
  ASSERT_TRUE(data.GetNamedField(L"six", &array));

  int64_t value = 0;
  TypedData element;
  for (size_t i = 0; i < arraysize(test_instance.six); ++i) {
    ASSERT_TRUE(array.GetArrayElement(i, &element));
    ASSERT_TRUE(element.GetSignedValue(&value));
    ASSERT_EQ(test_instance.six[i], value);
  }

  ASSERT_FALSE(array.GetArrayElement(arraysize(test_instance.six), &element));
}

}  // namespace refinery
