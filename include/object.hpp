#pragma once

#include <memory>
#include <string>

#include "chunk.hpp"

#define OBJECT_TYPE(value) (AS_OBJECT(value)->getType())

#define IS_FUNCTION(value) (IS_OBJECT(value) && OBJECT_TYPE(value) == OBJECT_FUNCTION)
#define IS_STRING(value) \
  (IS_OBJECT(value) && OBJECT_TYPE(value) == OBJECT_STRING)

#define AS_FUNCTION(value) \
  (std::static_pointer_cast<ObjectFunction>(AS_OBJECT(value)))
#define AS_OBJECTSTRING(value) \
  (std::static_pointer_cast<ObjectString>(AS_OBJECT(value)))
#define AS_STRING(value) \
  ((std::static_pointer_cast<ObjectString>(AS_OBJECT(value)))->getString())

enum ObjectType {
  OBJECT_FUNCTION,
  OBJECT_STRING
};

class Object {
 protected:
  const ObjectType type;

 public:
  Object(ObjectType type);
  ObjectType getType() const;

  void printObject() const;
};

class ObjectString : public Object {
  std::string str;
  size_t hash;

 public:
  ObjectString(std::string str);
  const std::string& getString() const;
  size_t getHash() const;

  struct Hash {
    size_t operator()(const std::shared_ptr<ObjectString>&) const;
  };

  struct Comparator {
    bool operator()(const std::shared_ptr<ObjectString>& a,
                    const std::shared_ptr<ObjectString>& b) const;
  };
};

class ObjectFunction : public Object {
  int arity;
  Chunk chunk;
  std::shared_ptr<ObjectString> name;

public:
  ObjectFunction(int arity, std::shared_ptr<ObjectString> name);
  const std::shared_ptr<ObjectString> getName() const;
  Chunk& getChunk();
};
