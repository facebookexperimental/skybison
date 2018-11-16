#include "gtest/gtest.h"

#include "layout.h"
#include "objects.h"
#include "runtime.h"

namespace python {

TEST(AttributeInfoTest, WithoutFlags) {
  AttributeInfo info(123, 0);
  EXPECT_EQ(info.offset(), 123);
  EXPECT_FALSE(info.isInObject());
}

TEST(AttributeInfoTest, WithFlags) {
  AttributeInfo info(123, AttributeInfo::Flag::kInObject);
  EXPECT_EQ(info.offset(), 123);
  EXPECT_TRUE(info.isInObject());
}

TEST(LayoutTest, FindAttribute) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Handle<Layout> layout(&scope, runtime.newLayout());

  // Should fail to find an attribute that isn't present
  Handle<Object> attr(&scope, runtime.newStringFromCString("myattr"));
  EXPECT_EQ(runtime.layoutFindAttribute(thread, layout, attr), Error::object());

  // Update the layout to include the new attribute as an in-object attribute
  Handle<ObjectArray> entry(&scope, runtime.newObjectArray(2));
  entry->atPut(0, *attr);
  entry->atPut(
      1, AttributeInfo(2222, AttributeInfo::Flag::kInObject).asSmallInteger());
  Handle<ObjectArray> array(&scope, runtime.newObjectArray(1));
  array->atPut(0, *entry);
  Layout::cast(*layout)->setInObjectAttributes(*array);

  // Should find the attribute
  Object* result = runtime.layoutFindAttribute(thread, layout, attr);
  ASSERT_TRUE(result->isSmallInteger());

  AttributeInfo info(result);
  EXPECT_EQ(info.offset(), 2222);
  EXPECT_TRUE(info.isInObject());
}

TEST(LayoutTest, AddNewAttributes) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Handle<Layout> layout(&scope, runtime.newLayout());

  // Should fail to find an attribute that isn't present
  Handle<Object> attr(&scope, runtime.newStringFromCString("myattr"));
  ASSERT_EQ(runtime.layoutFindAttribute(thread, layout, attr), Error::object());

  // Adding a new attribute should result in a new layout being created
  Handle<Layout> layout2(
      &scope, runtime.layoutAddAttribute(thread, layout, attr, 0));
  ASSERT_NE(*layout, *layout2);

  // Should be able find the attribute as an overflow attribute in the new
  // layout
  Object* info = runtime.layoutFindAttribute(thread, layout2, attr);
  ASSERT_FALSE(info->isError());
  EXPECT_TRUE(AttributeInfo(info).isOverflow());
  EXPECT_EQ(AttributeInfo(info).offset(), 0);

  // Adding another attribute should transition the layout again
  Handle<Object> attr2(&scope, runtime.newStringFromCString("another_attr"));
  ASSERT_EQ(
      runtime.layoutFindAttribute(thread, layout2, attr2), Error::object());
  Handle<Layout> layout3(
      &scope, runtime.layoutAddAttribute(thread, layout2, attr2, 0));
  ASSERT_NE(*layout2, *layout3);

  // We should be able to find both attributes in the new layout
  info = runtime.layoutFindAttribute(thread, layout3, attr);
  ASSERT_FALSE(info->isError());
  EXPECT_TRUE(AttributeInfo(info).isOverflow());
  EXPECT_EQ(AttributeInfo(info).offset(), 0);
  info = runtime.layoutFindAttribute(thread, layout3, attr2);
  ASSERT_FALSE(info->isError());
  EXPECT_TRUE(AttributeInfo(info).isOverflow());
  EXPECT_EQ(AttributeInfo(info).offset(), 1);
}

TEST(LayoutTest, AddDuplicateAttributes) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Handle<Layout> layout(&scope, runtime.newLayout());

  // Add an attribute
  Handle<Object> attr(&scope, runtime.newStringFromCString("myattr"));
  ASSERT_EQ(runtime.layoutFindAttribute(thread, layout, attr), Error::object());

  // Adding a new attribute should result in a new layout being created
  Handle<Layout> layout2(
      &scope, runtime.layoutAddAttribute(thread, layout, attr, 0));
  EXPECT_NE(*layout, *layout2);

  // Adding the attribute on the old layout should follow the edge and result
  // in the same layout being returned
  Handle<Layout> layout3(
      &scope, runtime.layoutAddAttribute(thread, layout, attr, 0));
  EXPECT_EQ(*layout2, *layout3);

  // Should be able to find the attribute in the new layout
  Object* info = runtime.layoutFindAttribute(thread, layout3, attr);
  ASSERT_TRUE(info->isSmallInteger());
  EXPECT_EQ(AttributeInfo(info).offset(), 0);
  EXPECT_TRUE(AttributeInfo(info).isOverflow());
}

TEST(LayoutTest, DeleteNonExistentAttribute) {
  Runtime runtime;
  HandleScope scope;
  Handle<Layout> layout(&scope, runtime.newLayout());
  Handle<Object> attr(&scope, runtime.newStringFromCString("myattr"));
  Thread* thread = Thread::currentThread();
  Object* result = runtime.layoutDeleteAttribute(thread, layout, attr);
  EXPECT_EQ(result, Error::object());
}

TEST(LayoutTest, DeleteInObjectAttribute) {
  Runtime runtime;
  HandleScope scope;

  // Create a new layout with a single in-object attribute
  Handle<Object> attr(&scope, runtime.newStringFromCString("myattr"));
  Handle<ObjectArray> entry(&scope, runtime.newObjectArray(2));
  entry->atPut(0, *attr);
  entry->atPut(
      1, AttributeInfo(2222, AttributeInfo::Flag::kInObject).asSmallInteger());
  Handle<ObjectArray> array(&scope, runtime.newObjectArray(1));
  array->atPut(0, *entry);
  Handle<Layout> layout(&scope, runtime.newLayout());
  layout->setInObjectAttributes(*array);

  // Deleting the attribute should succeed and return a new layout
  Thread* thread = Thread::currentThread();
  Object* result = runtime.layoutDeleteAttribute(thread, layout, attr);
  ASSERT_TRUE(result->isLayout());
  Handle<Layout> layout2(&scope, result);
  EXPECT_NE(layout->id(), layout2->id());

  // The new layout should have the entry for the attribute marked as deleted
  ASSERT_TRUE(layout2->inObjectAttributes()->isObjectArray());
  Handle<ObjectArray> inobject(&scope, layout2->inObjectAttributes());
  ASSERT_EQ(inobject->length(), 1);
  ASSERT_TRUE(inobject->at(0)->isObjectArray());
  entry = inobject->at(0);
  EXPECT_EQ(entry->at(0), None::object());
  ASSERT_TRUE(entry->at(1)->isSmallInteger());
  EXPECT_EQ(AttributeInfo(entry->at(1)).flags(), 2);

  // Performing the same deletion should follow the edge created by the
  // previous deletion and arrive at the same layout
  result = runtime.layoutDeleteAttribute(thread, layout, attr);
  ASSERT_TRUE(result->isLayout());
  Handle<Layout> layout3(&scope, result);
  EXPECT_EQ(*layout3, *layout2);
}

TEST(LayoutTest, DeleteOverflowAttribute) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  // Create a new layout with several overflow attributes
  Handle<Object> attr(&scope, runtime.newStringFromCString("myattr"));
  Handle<Object> attr2(&scope, runtime.newStringFromCString("myattr2"));
  Handle<Object> attr3(&scope, runtime.newStringFromCString("myattr3"));
  Handle<ObjectArray> attrs(&scope, runtime.newObjectArray(3));
  Handle<Object>* names[] = {&attr, &attr2, &attr3};
  for (word i = 0; i < attrs->length(); i++) {
    Handle<ObjectArray> entry(&scope, runtime.newObjectArray(2));
    entry->atPut(0, **names[i]);
    entry->atPut(1, AttributeInfo(i, 0).asSmallInteger());
    attrs->atPut(i, *entry);
  }
  Handle<Layout> layout(&scope, runtime.newLayout());
  layout->setOverflowAttributes(*attrs);

  // Delete the middle attribute. Make sure a new layout is created and the
  // entry after the deleted attribute has its offset updated correctly.
  Object* result = runtime.layoutDeleteAttribute(thread, layout, attr2);
  ASSERT_TRUE(result->isLayout());
  Handle<Layout> layout2(&scope, result);
  EXPECT_NE(layout2->id(), layout->id());
  // The first attribute should have the same offset
  result = runtime.layoutFindAttribute(thread, layout2, attr);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(AttributeInfo(result).offset(), 0);
  // The second attribute should not exist
  result = runtime.layoutFindAttribute(thread, layout2, attr2);
  EXPECT_TRUE(result->isError());
  // The third attribute should have been shifted down by 1
  result = runtime.layoutFindAttribute(thread, layout2, attr3);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(AttributeInfo(result).offset(), 1);

  // Delete the first attribute. A new layout should be created and the last
  // entry is shifted into the first position.
  result = runtime.layoutDeleteAttribute(thread, layout2, attr);
  ASSERT_TRUE(result->isLayout());
  Handle<Layout> layout3(&scope, result);
  EXPECT_NE(layout3->id(), layout->id());
  EXPECT_NE(layout3->id(), layout2->id());
  // The first attribute should not exist
  result = runtime.layoutFindAttribute(thread, layout3, attr);
  EXPECT_TRUE(result->isError());
  // The second attribute should not exist
  result = runtime.layoutFindAttribute(thread, layout3, attr2);
  EXPECT_TRUE(result->isError());
  // The third attribute should now occupy the first position
  result = runtime.layoutFindAttribute(thread, layout3, attr3);
  ASSERT_TRUE(result->isSmallInteger());
  EXPECT_EQ(AttributeInfo(result).offset(), 0);

  // Delete the remaining attribute. A new layout should be created and the
  // overflow array should be empty.
  result = runtime.layoutDeleteAttribute(thread, layout3, attr3);
  ASSERT_TRUE(result->isLayout());
  Handle<Layout> layout4(&scope, result);
  EXPECT_NE(layout4->id(), layout->id());
  EXPECT_NE(layout4->id(), layout2->id());
  EXPECT_NE(layout4->id(), layout3->id());
  // No attributes should exist
  result = runtime.layoutFindAttribute(thread, layout4, attr);
  EXPECT_TRUE(result->isError());
  result = runtime.layoutFindAttribute(thread, layout4, attr2);
  EXPECT_TRUE(result->isError());
  result = runtime.layoutFindAttribute(thread, layout4, attr3);
  EXPECT_TRUE(result->isError());
}

TEST(LayoutTest, DeleteAndAddInObjectAttribute) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  auto create_attrs = [&runtime](const Handle<Object>& name, uword flags) {
    HandleScope scope;
    Handle<ObjectArray> entry(&scope, runtime.newObjectArray(2));
    entry->atPut(0, *name);
    entry->atPut(1, AttributeInfo(0, flags).asSmallInteger());
    Handle<ObjectArray> result(&scope, runtime.newObjectArray(1));
    result->atPut(0, *entry);
    return *result;
  };

  // Create a new layout with one overflow attribute and one in-object
  // attribute
  Handle<Layout> layout(&scope, runtime.newLayout());
  Handle<Object> inobject(&scope, runtime.newStringFromCString("inobject"));
  layout->setInObjectAttributes(
      create_attrs(inobject, AttributeInfo::Flag::kInObject));
  Handle<Object> overflow(&scope, runtime.newStringFromCString("overflow"));
  layout->setOverflowAttributes(create_attrs(overflow, 0));

  // Delete the in-object attribute and add it back. It should be re-added as
  // an overflow attribute.
  Object* result = runtime.layoutDeleteAttribute(thread, layout, inobject);
  ASSERT_TRUE(result->isLayout());
  Handle<Layout> layout2(&scope, result);
  result = runtime.layoutAddAttribute(thread, layout2, inobject, 0);
  ASSERT_TRUE(result->isLayout());
  Handle<Layout> layout3(&scope, result);
  result = runtime.layoutFindAttribute(thread, layout3, inobject);
  ASSERT_TRUE(result->isSmallInteger());
  AttributeInfo info(result);
  EXPECT_EQ(info.offset(), 1);
  EXPECT_TRUE(info.isOverflow());
}

} // namespace python
