#include "vm.hpp"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>

#include "chunk.hpp"
#include "object.hpp"

#ifdef DEBUG
#include "debug.hpp"
#endif

VM::VM() { defineNative("clock", VM::clockNative); }

Value MemoryStack::getValueAt(size_t index) const { return c[index]; }

Value* MemoryStack::getValuePtrAt(size_t index) const {
  return (Value*)&(c[index]);
}

void MemoryStack::setValueAt(Value value, size_t index) { c[index] = value; }

InterpretResult VM::binaryOperation(char operation) {
  Value a = memory.top();
  memory.pop();
  Value b = memory.top();
  memory.pop();

  if (IS_NUM(a) && IS_NUM(b)) {
    double c = AS_NUM(a);
    double d = AS_NUM(b);

    switch (operation) {
      case '+':
        memory.push(NUM_VAL(d + c));
        break;
      case '-':
        memory.push(NUM_VAL(d - c));
        break;
      case '*':
        memory.push(NUM_VAL(d * c));
        break;
      case '/':
        memory.push(NUM_VAL(d / c));
        break;
      case '>':
        memory.push(BOOL_VAL(d > c));
        break;
      case '<':
        memory.push(BOOL_VAL(d < c));
        break;
      case '%':
        memory.push(NUM_VAL(fmod(d, c)));
        break;
      default:
        return INTERPRET_RUNTIME_ERROR;  // unreachable
    }
  } else if (IS_STRING(a) && IS_STRING(b)) {
    const std::string& c = AS_STRING(a);
    const std::string& d = AS_STRING(b);

    switch (operation) {
      case '+':
        concatenate(c, d);
        break;
      default:
        std::string symbol(1, operation);
        runtimeError("Invalid operation '%s' on strings.", symbol.c_str());
        return INTERPRET_RUNTIME_ERROR;
    }
  } else {
    runtimeError("Operands must be numbers or strings.");
    return INTERPRET_RUNTIME_ERROR;
  }
  return INTERPRET_OK;
}

void VM::resetMemory() { memory = MemoryStack(); }

void VM::runtimeError(const char* format, ...) {
#ifdef DEBUG
  printStack(memory);
#endif
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  while (!frames.empty()) {
    CallFrame& frame = frames.top();
    ObjectFunction& function = *(frame.closure.getFunction());

    std::cerr << "[line "
              << function.getChunk().getBytecodeAt(frame.PC - 1).line
              << "] in ";

    if (function.getName() == nullptr) {
      std::cerr << "script" << std::endl;
    } else {
      std::cerr << function.getName()->getString() << "()" << std::endl;
    }
    frames.pop();
  }

  std::cerr << "(Runtime Error)" << std::endl;
  resetMemory();
}

InterpretResult VM::interpret(std::shared_ptr<ObjectFunction> function) {
  std::shared_ptr<ObjectClosure> closure =
      std::make_shared<ObjectClosure>(function);
  memory.push(OBJECT_VAL(closure));
  callValue(OBJECT_VAL(closure), 0);
  return run();
}

InterpretResult VM::run() {
  CallFrame* frame = &(frames.top());
  while (true) {
    switch (readByte()) {
      case OP_CONSTANT: {
        Value constant = getTopChunk().getConstantAt(readByte());
        memory.push(constant);
        break;
      }
      case OP_NULL: {
        memory.push(NULL_VAL);
        break;
      }
      case OP_TRUE: {
        memory.push(BOOL_VAL(true));
        break;
      }
      case OP_FALSE: {
        memory.push(BOOL_VAL(false));
        break;
      }
      case OP_POP: {
        memory.pop();
        break;
      }
      case OP_GET_LOCAL: {
        uint8_t slot = readByte();
        memory.push(memory.getValueAt(slot + frame->stackPos));
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = readByte();
        memory.setValueAt(memory.top(), slot + frame->stackPos);
        break;
      }
      case OP_GET_GLOBAL: {
        Value constantName = readConstant();
        std::shared_ptr<ObjectString> name = AS_OBJECTSTRING(constantName);
        auto it = globals.find(name);
        if (it == globals.end()) {
          runtimeError("Undefined variable '%s'.", name->getString().c_str());
          return INTERPRET_RUNTIME_ERROR;
        }
        memory.push(it->second);
        break;
      }
      case OP_SET_GLOBAL: {
        Value constantName = readConstant();
        std::shared_ptr<ObjectString> name = AS_OBJECTSTRING(constantName);
        // std::cout << name->getString() << std::endl;
        globals.insert_or_assign(name, memory.top());
        break;
      }
      case OP_EQUAL: {
        Value a = memory.top();
        memory.pop();
        Value b = memory.top();
        memory.pop();
        memory.push(BOOL_VAL(a == b));
        break;
      }
      case OP_GREATER: {
        if (binaryOperation('>') == INTERPRET_RUNTIME_ERROR)
          return INTERPRET_RUNTIME_ERROR;
        break;
      }
      case OP_LESS: {
        if (binaryOperation('<') == INTERPRET_RUNTIME_ERROR)
          return INTERPRET_RUNTIME_ERROR;
        break;
      }
      case OP_ADD: {
        if (binaryOperation('+') == INTERPRET_RUNTIME_ERROR)
          return INTERPRET_RUNTIME_ERROR;
        break;
      }
      case OP_SUBSTRACT: {
        if (binaryOperation('-') == INTERPRET_RUNTIME_ERROR)
          return INTERPRET_RUNTIME_ERROR;
        break;
      }
      case OP_MULTIPLY: {
        if (binaryOperation('*') == INTERPRET_RUNTIME_ERROR)
          return INTERPRET_RUNTIME_ERROR;
        break;
      }
      case OP_DIVIDE: {
        if (binaryOperation('/') == INTERPRET_RUNTIME_ERROR)
          return INTERPRET_RUNTIME_ERROR;
        break;
      }
      case OP_MODULO: {
        if (binaryOperation('%') == INTERPRET_RUNTIME_ERROR)
          return INTERPRET_RUNTIME_ERROR;
        break;
      }
      case OP_NOT: {
        Value a = memory.top();
        memory.pop();
        memory.push(BOOL_VAL(isFalsey(a)));
        break;
      }
      case OP_NEGATE: {
        if (!IS_NUM(memory.top())) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        double a = AS_NUM(memory.top());
        memory.pop();
        memory.push(NUM_VAL(-a));
        break;
      }
      case OP_PRINT: {
        Value a = memory.top();
        memory.pop();
        a.printValue();
        std::cout << std::endl;
        break;
      }
      case OP_JUMP: {
        frame->PC += readShort();
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = readShort();
        if (isFalsey(memory.top())) frame->PC += offset;
        break;
      }
      case OP_LOOP: {
        frame->PC -= readShort();
        break;
      }
      case OP_CALL: {
        const int argCount = readByte();
        const size_t argStart = memory.size() - 1 - argCount;
        if (!callValue(memory.getValueAt(argStart), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &(frames.top());
        break;
      }
      case OP_CLOSURE: {
        std::shared_ptr<ObjectFunction> function = AS_FUNCTION(readConstant());
        std::shared_ptr<ObjectClosure> closure =
            std::make_shared<ObjectClosure>(function);
        memory.push(OBJECT_VAL(closure));
        for (int i = 0; i < closure->getUpvalueCount(); i++) {
          uint8_t isLocal = readByte();
          uint8_t index = readByte();
          if (isLocal) {
            closure->addUpvalue(
                captureUpvalue(memory.getValuePtrAt(frame->stackPos + index),
                               frame->stackPos + index));
          } else {
            closure->addUpvalue(frame->closure.getUpvalue(index));
          }
        }
        break;
      }
      case OP_GET_UPVALUE: {
        uint8_t slot = readByte();
        memory.push(*(frame->closure.getUpvalue(slot)->getLocation()));
        break;
      }
      case OP_SET_UPVALUE: {
        uint8_t slot = readByte();
        *(frame->closure.getUpvalue(slot)->getLocation()) = memory.top();
        break;
      }
      case OP_CLOSE_UPVALUE: {
        closeUpvalues(memory.size() - 1);
        memory.pop();
        break;
      }
      case OP_RETURN: {
        // retrieve and pop return value
        Value top = memory.top();
        memory.pop();
        closeUpvalues(frame->stackPos);

        // pop all local variables alongside function object
        size_t initialMemorySize = memory.size();
        for (size_t i = frames.top().stackPos; i < initialMemorySize; i++) {
          memory.pop();
        }

        // pop function frame
        frames.pop();

        if (frames.empty()) {
#ifdef DEBUG
          if (memory.size() != 0) {
            std::cout << "PANIC: STACK IS NOT EMPTY!" << std::endl << std::endl;
          }
          printStack(memory);
#endif
          if (memory.size() != 0) {
            runtimeError("Stack is not empty.");
            return INTERPRET_RUNTIME_ERROR;
          }
          return INTERPRET_OK;
        }

        // push return value on stack (for outer scope) and set next frame
        memory.push(top);
        frame = &(frames.top());
        break;
      }
    }
  }
  return INTERPRET_OK;
}

bool VM::call(std::shared_ptr<ObjectClosure> closure, int argCount) {
  if (argCount != closure->getFunction()->getArity()) {
    runtimeError("Expected %d arguments but found %d.",
                 closure->getFunction()->getArity(), argCount);
    return false;
  }

  if (frames.size() == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame newFrame{*closure, memory.size() - argCount - 1, 0};
  frames.push(newFrame);
  return true;
}

bool VM::callValue(Value callee, int argCount) {
  if (IS_OBJECT(callee)) {
    switch (OBJECT_TYPE(callee)) {
      case OBJECT_NATIVE: {
        NativeFn native = AS_NATIVE(callee)->getFunction();
        Value result = native(argCount, memory.size() - argCount);
        for (int i = 0; i <= argCount; i++) {
          memory.pop();
        }
        memory.push(result);
        return true;
      }
      case OBJECT_CLOSURE:
        return call(AS_CLOSURE(callee), argCount);
      default:
        break;
    }
  }

  runtimeError("Can only call functions and classes.");
  return false;
}

bool VM::isFalsey(Value value) const {
  return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value)) ||
         (IS_NUM(value) && !AS_NUM(value));
}

void VM::concatenate(const std::string& c, const std::string& d) {
  memory.push(OBJECT_VAL(std::make_shared<ObjectString>(d + c)));
}

Chunk& VM::getTopChunk() {
  return frames.top().closure.getFunction()->getChunk();
}

uint8_t VM::readByte() {
  uint8_t byte = getTopChunk().getBytecodeAt(frames.top().PC).code;
  frames.top().PC++;
  return byte;
}

Value VM::readConstant() { return getTopChunk().getConstantAt(readByte()); }

uint16_t VM::readShort() {
  uint8_t high = readByte();
  uint8_t lo = readByte();
  return (uint16_t)((high << 8) | lo);
}

void VM::defineNative(std::string name, NativeFn function) {
  std::shared_ptr<ObjectString> nativeName =
      std::make_shared<ObjectString>(name);
  globals.emplace(nativeName, OBJECT_VAL(std::make_shared<ObjectNative>(
                                  function, nativeName)));
}

Value VM::clockNative(int argCount, size_t index) {
  (void)argCount;
  (void)index;
  return NUM_VAL((double)clock() / CLOCKS_PER_SEC);
}

std::shared_ptr<ObjectUpvalue> VM::captureUpvalue(Value* local,
                                                  int localIndex) {
  std::shared_ptr<ObjectUpvalue> prevUpvalue = nullptr;
  std::shared_ptr<ObjectUpvalue> upvalue = openUpvalues;
  while (upvalue != nullptr && upvalue->getLocationIndex() > localIndex) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }
  if (upvalue != nullptr && upvalue->getLocation() == local) {
    return upvalue;
  }

  std::shared_ptr<ObjectUpvalue> toReturn =
      std::make_shared<ObjectUpvalue>(local, localIndex);
  toReturn->next = upvalue;

  if (prevUpvalue == nullptr) {
    openUpvalues = toReturn;
  } else {
    prevUpvalue->next = toReturn;
  }

  return toReturn;
}

void VM::closeUpvalues(int lastIndex) {
  while (openUpvalues != nullptr &&
         openUpvalues->getLocationIndex() >= lastIndex) {
    std::shared_ptr<ObjectUpvalue> upvalue = openUpvalues;
    upvalue->closed = *(upvalue->getLocation());
    upvalue->location = &(upvalue->closed.value());
    openUpvalues = upvalue->next;
  }
}