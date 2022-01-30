#include "object.hpp"

#include <iostream>

Object::Object(ObjectType type) : type{type} {}

ObjectType Object::getType() const { return type; }

ObjectString::ObjectString(const std::string& str)
    : Object(OBJECT_STRING), str{str} {
  hash = std::hash<std::string>{}(str);
}

const std::string& ObjectString::getString() const { return str; }

size_t ObjectString::getHash() const { return hash; }

void Object::printObject() const {
  switch (type) {
    case OBJECT_CLOSURE: {
      ((ObjectClosure*)this)->getFunction()->printObject();
      break;
    }
    case OBJECT_FUNCTION: {
      if (((ObjectFunction*)this)->getName() == nullptr) {
        std::cout << "<script>";
      } else {
        std::cout << ((ObjectFunction*)this)->getName()->getString();
      }
      break;
    }
    case OBJECT_STRING: {
      std::cout << ((ObjectString*)this)->getString();
      break;
    }
    case OBJECT_NATIVE: {
      std::cout << ((ObjectNative*)this)->getName()->getString();
      break;
    }
    case OBJECT_UPVALUE: {
      std::cout << "upvalue" << std::endl;
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

ObjectFunction::ObjectFunction(std::shared_ptr<ObjectString> name)
    : Object(OBJECT_FUNCTION), arity{0}, name{name} {}

const std::shared_ptr<ObjectString> ObjectFunction::getName() const {
  return name;
}

Chunk& ObjectFunction::getChunk() { return chunk; }

void ObjectFunction::increaseArity() { arity++; }

int ObjectFunction::getArity() const { return arity; }

ObjectNative::ObjectNative(const NativeFn function,
                           const std::shared_ptr<ObjectString> name)
    : Object(OBJECT_NATIVE), function{function}, name{name} {}

NativeFn ObjectNative::getFunction() { return function; }

std::shared_ptr<ObjectString> ObjectNative::getName() { return name; }

ObjectClosure::ObjectClosure(std::shared_ptr<ObjectFunction> function)
    : Object(OBJECT_CLOSURE), function{function} {
  upvalueCount = function->getUpvalueCount();
}

std::shared_ptr<ObjectFunction> ObjectClosure::getFunction() {
  return function;
}

size_t ObjectClosure::getUpvaluesSize() const { return upvalues.size(); }

void ObjectClosure::setUpvalue(int index,
                               std::shared_ptr<ObjectUpvalue> upvalue) {
  upvalues[index] = upvalue;
}

std::shared_ptr<ObjectUpvalue> ObjectClosure::getUpvalue(int index) const {
  return upvalues[index];
}

ObjectUpvalue::ObjectUpvalue(Value* location, int locationIndex)
    : Object(OBJECT_UPVALUE),
      locationIndex{locationIndex},
      location{location} {}

Value* ObjectUpvalue::getLocation() const { return location; }

void ObjectFunction::increateUpvalueCount() { upvalueCount++; }

int ObjectFunction::getUpvalueCount() const { return upvalueCount; }

int ObjectClosure::getUpvalueCount() const { return upvalueCount; }

void ObjectClosure::addUpvalue(std::shared_ptr<ObjectUpvalue> upvalue) {
  upvalues.push_back(upvalue);
}

int ObjectUpvalue::getLocationIndex() const { return locationIndex; }