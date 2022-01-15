#include "object.hpp"

#include <iostream>

Object::Object(ObjectType type) : type{type} {}

ObjectType Object::getType() const { return type; }

ObjectString::ObjectString(std::string str) : Object(OBJECT_STRING), str{str} {
  hash = std::hash<std::string>{}(str);
}

const std::string& ObjectString::getString() const { return str; }

size_t ObjectString::getHash() const { return hash; }

void Object::printObject() const {
  switch (type) {
    case OBJECT_FUNCTION: {
      if (((ObjectFunction*)this)->getName() == nullptr) {
        std::cout << "<script>" << std::endl;
      } else {
        std::cout << ((ObjectFunction*)this)->getName()->getString();
      }
      break;
    }
    case OBJECT_STRING: {
      std::cout << ((ObjectString*)this)->getString();
      break;
    }
  }
}

size_t ObjectString::Hash::operator()(
    const std::shared_ptr<ObjectString>& a) const {
  return a->getHash();
}

bool ObjectString::Comparator::operator()(
    const std::shared_ptr<ObjectString>& a,
    const std::shared_ptr<ObjectString>& b) const {
  return a->getString() == b->getString();
}

ObjectFunction::ObjectFunction(int arity, std::shared_ptr<ObjectString> name)
    : Object(OBJECT_FUNCTION), arity{arity}, name{name} {}

const std::shared_ptr<ObjectString> ObjectFunction::getName() const {
  (void)arity;
  return name;
}

Chunk& ObjectFunction::getChunk() { return chunk; }