// ir.h - SSA IR Types and Values

#pragma once

#include <iostream>
#include <vector>
#include <cstring>
#include <type_traits>
#include <memory>
#include <string>
#include <cassert>
#include <unordered_map>
#include <algorithm>

#include "mo_debug.h"

//===----------------------------------------------------------------------===//
//                             Forward Declarations
//===----------------------------------------------------------------------===//

class Type;
class IntegerType;
class FloatType;
class VoidType;
class PointerType;
class FunctionType;
class ArrayType;
class StructType;
class VectorType;
class QualifiedType;
class Value;
class User;
enum class Opcode;
class Instruction;
class BasicBlock;
class Function;
class Argument;
class Module;
class Constant;
class ConstantInt;
class ConstantFP;
class ConstantAggregate;
class ConstantArray;
class ConstantString;
class ConstantStruct;
class GlobalVariable;
class ConstantPointerNull;
class ConstantAggregateZero;
class BranchInst;
class ReturnInst;
class PhiInst;
class ICmpInst;
class FCmpInst;
class AllocaInst;
class LoadInst;
class StoreInst;
class GetElementPtrInst;
class BinaryInst;
class ConversionInst;
class CastInst;
class BitCastInst;
class CallInst;
class RawCallInst;
class SExtInst;
class TruncInst;
class SIToFPInst;
class FPToSIInst;
class FPExtInst;
class FPTruncInst;

struct Member;
struct StructLayout;

StructLayout calculate_aligned_layout(const std::vector<Type *> &members);
uint64_t truncate_value(uint64_t value, uint8_t bit_width, bool is_unsigned);

//===----------------------------------------------------------------------===//
//                              Utility Classes
//===----------------------------------------------------------------------===//
namespace std
{
    template <>
    struct hash<std::pair<Type *, uint64_t>>
    {
        size_t operator()(const std::pair<Type *, uint64_t> &p) const
        {
            return hash<Type *>{}(p.first) ^ (hash<uint64_t>{}(p.second) << 1);
        }
    };

    template <>
    struct hash<std::pair<uint8_t, bool>>
    {
        std::size_t operator()(const std::pair<uint8_t, bool> &k) const
        {
            return ((hash<uint8_t>()(k.first) ^ (hash<bool>()(k.second) << 1)));
        }
    };
};

enum class Qualifier : uint8_t
{
    None = 0,
    Const = 1,
    Volatile = 1 << 1,
    Restrict = 1 << 2
};

constexpr Qualifier operator|(Qualifier a, Qualifier b) noexcept
{
    return static_cast<Qualifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr Qualifier operator&(Qualifier a, Qualifier b) noexcept
{
    return static_cast<Qualifier>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

//===----------------------------------------------------------------------===//
//                               Type System
//===----------------------------------------------------------------------===//
using ParamList = std::vector<std::pair<std::string, Type *>>;

std::vector<Type *> param_list_to_types(const ParamList &params);

class Type
{
public:
    enum TypeID
    {
        VoidTy,
        IntTy,
        FpTy,
        PtrTy,
        FuncTy,
        ArrayTy,
        StructTy,
        VecTy,
        QualifierTy,
    };
    static const char *id_to_str(TypeID id)
    {
        switch (id)
        {
        case VoidTy:
            return "void";
        case IntTy:
            return "integer";
        case FpTy:
            return "float";
        case PtrTy:
            return "pointer";
        case FuncTy:
            return "function";
        case ArrayTy:
            return "array";
        case StructTy:
            return "struct";
        case VecTy:
            return "vector";
        case QualifierTy:
            return "qualifier";
        default:
            return "unknown";
        }
    }

    Type(TypeID tid, Module *m) : tid_(tid), module_(m) {}
    virtual ~Type() {};

    TypeID type_id() const { return tid_; }
    virtual size_t size() const = 0;
    virtual std::string name() const = 0;
    virtual uint8_t bit_width() const = 0;
    // TODO: for different architectures
    virtual size_t alignment() const
    {
        return (bit_width() + 7) / 8;
    }

    Module *module() const { return module_; }

    static Type *get_void_type(Module *m);

    virtual bool is_void() const { return false; }
    virtual bool is_float() const { return false; }
    virtual bool is_integer() const { return false; }
    virtual bool is_pointer() const { return false; }
    virtual bool is_function() const { return false; }
    virtual bool is_array() const { return false; }
    virtual bool is_struct() const { return false; }
    virtual bool is_tuple() const { return false; }
    virtual bool is_vector() const { return false; }
    virtual bool is_qualified() const { return false; }

    virtual bool is_scalar() const noexcept { return false; }
    virtual bool is_numeric() const noexcept { return false; }
    virtual bool is_aggregate() const noexcept { return false; }
    virtual bool is_unsigned() const noexcept { return false; }

    // Type conversion methods
    virtual IntegerType *as_integer() noexcept { return nullptr; }
    virtual const IntegerType *as_integer() const noexcept { return nullptr; }
    virtual FloatType *as_float() noexcept { return nullptr; }
    virtual const FloatType *as_float() const noexcept { return nullptr; }
    virtual PointerType *as_pointer() noexcept { return nullptr; }
    virtual const PointerType *as_pointer() const noexcept { return nullptr; }
    virtual ArrayType *as_array() noexcept { return nullptr; }
    virtual const ArrayType *as_array() const noexcept { return nullptr; }
    virtual VectorType *as_vector() noexcept { return nullptr; }
    virtual const VectorType *as_vector() const noexcept { return nullptr; }
    virtual FunctionType *as_function() noexcept { return nullptr; }
    virtual const FunctionType *as_function() const noexcept { return nullptr; }
    virtual StructType *as_struct() noexcept { return nullptr; }
    virtual const StructType *as_struct() const noexcept { return nullptr; }
    virtual QualifiedType *as_qualified() noexcept { return nullptr; }
    virtual const QualifiedType *as_qualified() const noexcept { return nullptr; }

    Type *element_type() const;

    // Equality comparison
    bool operator==(const Type &other) const
    {
        if (this == &other)
            return true;
        if (type_id() != other.type_id())
            return false;

        return is_equal(other);
    }

    bool operator!=(const Type &other) const { return !(*this == other); }

    virtual std::string to_string() const
    {
        return name();
    }

protected:
    virtual bool is_equal(const Type &other) const
    {
        assert(false && "is_equal not implemented for this type");
        return false;
    }

protected:
    TypeID tid_;
    Module *module_;
};

//===----------------------------------------------------------------------===//
//                             Abstract Base Class for Scalar Types
//===----------------------------------------------------------------------===//
class ScalarType : public Type
{
public:
    bool is_scalar() const noexcept override { return true; }

protected:
    ~ScalarType() = default; // Prevent direct instantiation
    ScalarType(TypeID tid, Module *m) : Type(tid, m) {}
};

//===----------------------------------------------------------------------===//
//                             Abstract Base Class for Numeric Types
//===----------------------------------------------------------------------===//
class NumericType : public ScalarType
{
public:
    bool is_numeric() const noexcept override { return true; }

protected:
    ~NumericType() = default; // Prevent direct instantiation
    NumericType(TypeID tid, Module *m) : ScalarType(tid, m) {}
};

//===----------------------------------------------------------------------===//
//                             Abstract Base Class for Aggregate Types
//===----------------------------------------------------------------------===//
class AggregateType : public Type
{
public:
    bool is_aggregate() const noexcept override { return true; }

protected:
    ~AggregateType() = default; // Prevent direct instantiation
    AggregateType(TypeID tid, Module *m) : Type(tid, m) {}
};

class IntegerType final : public NumericType
{
public:
    size_t size() const override { return (bit_width_ + 7) / 8; }
    std::string name() const override { return (unsigned_ ? "u" : "i") + std::to_string(bit_width_); }
    uint8_t bit_width() const override { return bit_width_; }
    bool is_unsigned() const noexcept override { return unsigned_; }

    IntegerType *as_integer() noexcept override { return this; }
    const IntegerType *as_integer() const noexcept override { return this; }

    bool is_integer() const noexcept override
    {
        return true;
    }

    std::string to_string() const override
    {
        return name();
    }

private:
    explicit IntegerType(Module *m, uint8_t bit_width, bool unsigned_ = false);
    uint8_t bit_width_;
    bool unsigned_ = false;

    friend Module;

protected:
    bool is_equal(const Type &other) const override
    {
        if (const IntegerType *other_int = dynamic_cast<const IntegerType *>(&other))
        {
            return bit_width_ == other_int->bit_width_ && unsigned_ == other_int->unsigned_;
        }
        return false;
    }
};

class FloatType final : public NumericType
{
public:
    uint8_t bit_width() const override { return bit_width_; }
    size_t size() const override { return (bit_width_ + 7) / 8; }
    std::string name() const override { return "f" + std::to_string(bit_width_); }

    FloatType *as_float() noexcept override { return this; }
    const FloatType *as_float() const noexcept override { return this; }
    bool is_float() const noexcept override
    {
        return true;
    }

    std::string to_string() const override
    {
        return name();
    }

private:
    explicit FloatType(Module *m, uint8_t bit_width);
    uint8_t bit_width_;
    friend class Module;

protected:
    bool is_equal(const Type &other) const override
    {
        if (const FloatType *other_float = dynamic_cast<const FloatType *>(&other))
        {
            return bit_width_ == other_float->bit_width_;
        }
        return false;
    }
};

class VoidType final : public Type
{
public:
    size_t size() const override { return 0; }
    std::string name() const override { return "void"; }
    uint8_t bit_width() const override { return 0; }

    bool is_void() const noexcept override
    {
        return true;
    }

    std::string to_string() const override
    {
        return name();
    }

private:
    explicit VoidType(Module *m) : Type(VoidTy, m) {}

    Module *module_;

    friend Module;

protected:
    bool is_equal(const Type &other) const override
    {
        return dynamic_cast<const VoidType *>(&other) != nullptr;
    }
};

class PointerType final : public Type
{
public:
    Type *element_type() const { return element_type_; }

    // FIXME: Size should be handled through DataLayout instead of hardcoding
    size_t size() const override { return sizeof(void *); }
    std::string name() const override { return element_type_->name() + "*"; }
    uint8_t bit_width() const override { return sizeof(void *) * 8; }

    PointerType *as_pointer() noexcept override { return this; }
    const PointerType *as_pointer() const noexcept override { return this; }
    bool is_pointer() const noexcept override
    {
        return true;
    }

    std::string to_string() const override
    {
        return element_type_->to_string() + "*";
    }

private:
    PointerType(Module *m, Type *element_type);

    Type *element_type_;
    Module *module_;

    friend Module;

protected:
    bool is_equal(const Type &other) const override
    {
        if (const PointerType *other_ptr = dynamic_cast<const PointerType *>(&other))
        {
            return *element_type_ == *other_ptr->element_type_;
        }
        return false;
    }
};

class FunctionType final : public Type
{
public:
    Type *return_type() const { return return_type_; }

    const std::vector<Type *> param_types() const
    {
        std::vector<Type *> result;
        for (auto &p : params_)
        {
            result.push_back(p.second);
        }
        return result;
    }
    const Type *param_type(unsigned index) const { return params_[index].second; }
    const std::string &param_name(unsigned index) const { return params_[index].first; }

    ParamList params() const { return ParamList(params_.begin(), params_.end()); }
    size_t num_params() const { return params_.size(); }
    size_t size() const override { return 0; }

    std::string name() const override
    {
        std::string result = return_type_->name() + " (";
        for (size_t i = 0; i < params_.size(); ++i)
        {
            if (i != 0)
                result += ", ";
            result += params_[i].second->name();
        }
        result += ")";
        return result;
    }

    uint8_t bit_width() const override { return 0; }

    FunctionType *as_function() noexcept override { return this; }
    const FunctionType *as_function() const noexcept override { return this; }
    bool is_function() const noexcept override
    {
        return true;
    }

    std::string to_string() const override
    {
        printf("return_type_ ptr addr: %p\n", return_type_);
        std::string result = return_type_->to_string() + " (";
        for (size_t i = 0; i < params_.size(); ++i)
        {
            if (i != 0)
                result += ", ";
            result += params_[i].second->to_string();
        }
        result += ")";
        return result;
    }

private:
    FunctionType(Module *m, Type *return_type, const ParamList &params)
        : Type(FuncTy, m), return_type_(return_type), params_(params.begin(), params.end())
    {
        for (size_t i = 0; i < params_.size(); ++i)
        {
            if (params_[i].first.empty())
            {
                params_[i].first = "__arg" + std::to_string(i);
            }
        }
    }

    Type *return_type_;
    ParamList params_;

    friend class Module;

protected:
    bool is_equal(const Type &other) const override
    {
        if (const FunctionType *other_func = dynamic_cast<const FunctionType *>(&other))
        {
            if (*return_type_ != *other_func->return_type_)
            {
                return false;
            }
            if (params_.size() != other_func->params_.size())
            {
                return false;
            }
            for (size_t i = 0; i < params_.size(); ++i)
            {
                if (*params_[i].second != *other_func->params_[i].second)
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};

class ArrayType final : public AggregateType
{
public:
    Type *element_type() const { return element_type_; }
    uint64_t num_elements() const { return num_elements_; }
    size_t size() const override;
    std::string name() const override
    {
        return "[" + std::to_string(num_elements_) + " x " + element_type_->name() + "]";
    }

    uint8_t bit_width() const override { return size() * 8; }

    ArrayType *as_array() noexcept override { return this; }
    const ArrayType *as_array() const noexcept override { return this; }
    bool is_array() const noexcept override
    {
        return true;
    }

    std::string to_string() const override
    {
        return "[" + std::to_string(num_elements_) + " x " + element_type_->to_string() + "]";
    }

private:
    ArrayType(Module *m, Type *element_type, uint64_t num_elements);

    Type *element_type_;
    uint64_t num_elements_;
    Module *module_;

    friend Module;

protected:
    bool is_equal(const Type &other) const override
    {
        if (const ArrayType *other_array = dynamic_cast<const ArrayType *>(&other))
        {
            return (num_elements_ == other_array->num_elements_) && (*element_type_ == *other_array->element_type_);
        }
        return false;
    }
};

struct MemberInfo
{
    std::string name;
    Type *type;
    MemberInfo(const std::string &name, Type *type) : name(name), type(type) {}

    bool operator==(const MemberInfo &other) const
    {
        return name == other.name && type == other.type;
    }

    bool operator!=(const MemberInfo &other) const
    {
        return !(*this == other);
    }
};

class StructType final : public AggregateType
{
public:
    friend class Module;
    // For named structs, identifier is the name
    // For anonymous structs, identifier is all members' types

    void set_name(const std::string &name) { name_ = name; }
    std::string name() const override
    {
        return "%" + name_;
    }
    std::string identifier() const { return name_; }

    uint8_t bit_width() const override { return size() * 8; }
    size_t alignment() const override
    {
        return 8; // TODO: handle alignment properly
    }

    // Completes the struct definition with member names and types
    void set_body(const std::vector<MemberInfo> &members);

    // Gets a member by index
    Type *get_member_type(unsigned index) const;

    // Gets the member offset
    size_t get_member_offset(unsigned index) const;

    size_t get_member_index(const std::string &name) const;
    bool has_member(const std::string &name) const;

    size_t size() const override;

    bool is_opaque() const { return is_opaque_; }
    bool is_tuple() const { return is_tuple_; }
    const std::vector<MemberInfo> &members() const { return members_; }

    StructType *as_struct() noexcept override { return this; }
    const StructType *as_struct() const noexcept override { return this; }
    bool is_struct() const noexcept override
    {
        return true;
    }

    std::string to_string() const override
    {
        if (is_opaque_)
            return "opaque";
        std::string result = "{ ";
        for (size_t i = 0; i < members_.size(); ++i)
        {
            if (i != 0)
                result += ", ";
            result += members_[i].type->to_string();
        }
        result += " }";
        return result;
    }

private:
    StructType(Module *m, const std::string &name, const std::vector<MemberInfo> &members);
    StructType(Module *m, const std::vector<MemberInfo> &members);

    std::string name_;
    Module *module_;
    bool is_opaque_; // true if forward declaration
    bool is_tuple_ = false;
    std::vector<MemberInfo> members_;
    std::vector<size_t> offsets_;
    size_t size_;

    friend Module;

protected:
    bool is_equal(const Type &other) const override
    {
        if (const StructType *other_struct = dynamic_cast<const StructType *>(&other))
        {
            if (is_opaque_ != other_struct->is_opaque_)
            {
                return false;
            }

            if (members_.size() != other_struct->members_.size())
            {
                return false;
            }

            for (size_t i = 0; i < members_.size(); ++i)
            {
                // NOTE: name is not checked here
                if (*members_[i].type != *other_struct->members_[i].type)
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};

class VectorType final : public AggregateType
{
public:
    Type *element_type() const { return element_type_; }
    uint64_t num_elements() const { return num_elements_; }

    size_t size() const override
    {
        assert(element_type_ && "Invalid element type");
        return element_type_->size() * num_elements_;
    }

    std::string name() const override
    {
        return "<" + std::to_string(num_elements_) + " x " + element_type_->name() + ">";
    }

    uint8_t bit_width() const override
    {
        return element_type_->bit_width() * num_elements_;
    }

    VectorType *as_vector() noexcept override { return this; }
    const VectorType *as_vector() const noexcept override { return this; }
    bool is_vector() const noexcept override
    {
        return true;
    }

    std::string to_string() const override
    {
        return "<" + std::to_string(num_elements_) + " x " + element_type_->to_string() + ">";
    }

private:
    VectorType(Module *m, Type *element_type, uint64_t num_elements)
        : AggregateType(VecTy, m), element_type_(element_type), num_elements_(num_elements) {}

    Type *element_type_;
    uint64_t num_elements_;

    friend class Module;

protected:
    bool is_equal(const Type &other) const override
    {
        if (const VectorType *other_vec = dynamic_cast<const VectorType *>(&other))
        {
            return (num_elements_ == other_vec->num_elements_) && (*element_type_ == *other_vec->element_type_);
        }
        return false;
    }
};

class QualifiedType final : public Type
{
public:
    QualifiedType(Qualifier q, Type *base)
        : Type(TypeID::QualifierTy, base->module()), qualifiers_(q), base_(std::move(base)) {}

    // Type characteristics
    Qualifier qualifiers() const noexcept { return qualifiers_; }
    const Type &base_type() const noexcept { return *base_; }
    Type &base_type() noexcept { return *base_; }

    // Type properties
    size_t size() const override { return base_->size(); }
    std::string name() const override { return base_->name(); }
    uint8_t bit_width() const override { return base_->bit_width(); }

    QualifiedType *as_qualified() noexcept override { return this; }
    const QualifiedType *as_qualified() const noexcept override { return this; }

    bool is_void() const noexcept override { return base_->is_void(); }
    bool is_float() const noexcept override { return base_->is_float(); }
    bool is_integer() const noexcept override { return base_->is_integer(); }
    bool is_pointer() const noexcept override { return base_->is_pointer(); }
    bool is_function() const noexcept override { return base_->is_function(); }
    bool is_array() const noexcept override { return base_->is_array(); }
    bool is_struct() const noexcept override { return base_->is_struct(); }
    bool is_tuple() const noexcept override { return base_->is_tuple(); }
    bool is_vector() const noexcept override { return base_->is_vector(); }
    bool is_qualified() const noexcept override { return is_qualified(); }

    std::string to_string() const override
    {
        std::string qual_str;
        if ((qualifiers_ & Qualifier::Const) != Qualifier::None)
        {
            qual_str += "const ";
        }
        if ((qualifiers_ & Qualifier::Volatile) != Qualifier::None)
        {
            qual_str += "volatile ";
        }
        return qual_str + base_->to_string();
    }

private:
    Qualifier qualifiers_;
    Type *base_;

protected:
    bool is_equal(const Type &other) const override
    {
        if (const QualifiedType *other_qualified = dynamic_cast<const QualifiedType *>(&other))
        {
            return (qualifiers_ == other_qualified->qualifiers_) && (*base_ == *other_qualified->base_);
        }
        return false;
    }
};
//===----------------------------------------------------------------------===//
//                              Value Base Class
//===----------------------------------------------------------------------===//
class Value
{
public:
    virtual ~Value();

    const std::string &name() const { return name_; }
    Type *type() const { return type_; }
    const std::vector<User *> &users() const { return users_; }

    void set_name(const std::string &name) { name_ = name; }
    void add_user(User *user) { users_.push_back(user); }
    void remove_user(Value *user);

protected:
    Value(Type *type, const std::string &name = "")
        : type_(type), name_(name) {}

    Type *type_;
    std::string name_;
    std::vector<User *> users_;
};

//===----------------------------------------------------------------------===//
//                              User Base Class
//===----------------------------------------------------------------------===//
class User : public Value
{
public:
    const std::vector<Value *> &operands() const { return operands_; }
    Value *operand(unsigned i) const
    {
        if (i >= operands_.size())
        {
            MO_WARN("Invalid operand index: %u", i);
            return nullptr;
        }
        return operands_.at(i);
    }
    void set_operand(unsigned i, Value *v);
    void remove_use_of(Value *v);

protected:
    User(Type *type, const std::string &name = "")
        : Value(type, name) {}
    ~User() override;

    std::vector<Value *> operands_;
};

//===----------------------------------------------------------------------===//
//                              Instruction System
//===----------------------------------------------------------------------===//
enum class Opcode
{
    // Math Operations
    Add,  // Addition
    Sub,  // Subtraction
    Mul,  // Multiplication
    UDiv, // Unsigned Division
    SDiv, // Signed Division
    URem, // Unsigned Remainder
    SRem, // Signed Remainder

    Neg,  // Negation
    Not,  // Logical Not
    FNeg, // Floating Negation

    // Memory Operations
    Alloca,        // Allocate memory
    Load,          // Load from memory
    Store,         // Store to memory
    GetElementPtr, // Get element pointer

    // Comparison Operations
    ICmp, // Integer comparison
    FCmp, // Floating-point comparison

    // Control Flow
    Br,          // Unconditional Branch
    CondBr,      // Conditional Branch
    Ret,         // Return
    Unreachable, // Placeholder for unreachable merge block
    Phi,         // Phi node

    // Function Call
    Call, // Function Call

    // Type Conversions
    ZExt,     // Zero Extend
    SExt,     // Sign Extend
    Trunc,    // Truncate
    SIToFP,   // Signed Integer to Floating-Point
    FPToSI,   // Floating-Point to Signed Integer
    FPExt,    // Floating-Point Extend
    FPTrunc,  // Floating-Point Truncate
    BitCast,  // Bit Cast
    PtrToInt, // Pointer to Integer
    IntToPtr, // Integer to Pointer
    FPToUI,   // Floating-Point to Unsigned Integer
    UIToFP,   // Unsigned Integer to Floating-Point

    // Bitwise Operations
    BitAnd, // Bitwise AND
    BitOr,  // Bitwise OR
    BitXor, // Bitwise XOR
    BitNot, // Bitwise Not

    Shl,
    LShr,
    AShr,

};

class Instruction : public User
{
public:
    Opcode opcode() const { return opcode_; }
    BasicBlock *parent() const { return parent_; }

    Instruction *next() const { return next_; }
    Instruction *prev() const { return prev_; }

    static Instruction *create(Opcode opc, Type *type,
                               std::vector<Value *> operands,
                               BasicBlock *parent);

protected:
    friend class BasicBlock;

    Instruction(Opcode opcode, Type *type, BasicBlock *parent,
                std::vector<Value *> operands, const std::string &name = "");

    Opcode opcode_;
    BasicBlock *parent_;
    Instruction *prev_;
    Instruction *next_;
};

//===----------------------------------------------------------------------===//
//                              Basic Block
//===----------------------------------------------------------------------===//
class BasicBlock : public Value
{
public:
    explicit BasicBlock(const std::string &name, Function *parent);
    ~BasicBlock() override;

    Function *parent_function() const { return parent_; }

    Instruction *first_instruction() const { return head_; }
    Instruction *last_instruction() const { return tail_; }
    Instruction *first_non_phi() const;
    Instruction *last_non_phi() const;
    Instruction *get_terminator() const;
    void insert_before(Instruction *pos, std::unique_ptr<Instruction> inst);
    void insert_after(Instruction *pos, std::unique_ptr<Instruction> inst);

    const std::vector<BasicBlock *> &predecessors() const { return predecessors_; }
    const std::vector<BasicBlock *> &successors() const { return successors_; }
    void add_successor(BasicBlock *bb);
    void append(Instruction *inst);

    class iterator
    {
    public:
        iterator(Instruction *ptr) : ptr_(ptr) {}

        Instruction &operator*() const { return *ptr_; }
        Instruction *operator->() { return ptr_; }
        iterator &operator++()
        {
            ptr_ = ptr_->next();
            return *this;
        }
        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator &other) const { return ptr_ == other.ptr_; }
        bool operator!=(const iterator &other) const { return ptr_ != other.ptr_; }

    private:
        Instruction *ptr_;
    };

    class const_iterator
    {
    public:
        const_iterator(const Instruction *ptr) : ptr_(ptr) {}

        const Instruction &operator*() const { return *ptr_; }
        const Instruction *operator->() { return ptr_; }
        const_iterator &operator++()
        {
            ptr_ = ptr_->next();
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator &other) const { return ptr_ == other.ptr_; }
        bool operator!=(const const_iterator &other) const { return ptr_ != other.ptr_; }

    private:
        const Instruction *ptr_;
    };

    iterator begin() { return iterator(head_); }
    iterator end() { return iterator(nullptr); }

    const_iterator begin() const { return const_iterator(head_); }
    const_iterator end() const { return const_iterator(nullptr); }

    using value_type = Instruction;
    using reference = Instruction &;
    using const_reference = const Instruction &;

private:
    Function *parent_;
    Instruction *head_;
    Instruction *tail_;

    std::vector<BasicBlock *> predecessors_;
    std::vector<BasicBlock *> successors_;
};

//===----------------------------------------------------------------------===//
//                              Function
//===----------------------------------------------------------------------===//
class Argument : public Value
{
public:
    Argument(const std::string &name, Type *type, Function *parent)
        : Value(type, name), parent_(parent) {}

    Function *parent() const { return parent_; }

private:
    Function *parent_;
};

class Function : public Value
{
public:
    Function(const std::string &name, Module *parent, Type *return_type,
             const ParamList &params);
    ~Function() override;

    BasicBlock *create_basic_block(const std::string &name = "");
    Module *parent_module() const { return parent_; }

    Type *return_type() const { return return_type_; }
    Type *arg_type(size_t idx) const { return args_.at(idx)->type(); }
    const std::vector<Argument *> &args() const { return args_; }
    Argument *arg(size_t idx) const { return args_.at(idx); }
    size_t num_args() const { return args_.size(); }
    void set_instance_method(bool is_instance_method) { is_instance_method_ = is_instance_method; }
    std::vector<Type *> param_types() const
    {
        std::vector<Type *> types;
        for (auto arg : args_)
            types.push_back(arg->type());
        return types;
    }
    auto begin() { return basic_blocks_.begin(); }
    auto end() { return basic_blocks_.end(); }
    auto entry_block() const { return basic_blocks_.front().get(); }
    bool remove_basic_block(BasicBlock *bb);

    bool has_hidden_retval() const { return has_hidden_retval_; }
    Type *hidden_retval_type() const { return hidden_retval_type_; }
    void set_hidden_retval(Type *retval_type)
    {
        has_hidden_retval_ = retval_type != nullptr;
        hidden_retval_type_ = retval_type;
    }

    const std::vector<BasicBlock *> &basic_blocks() const { return basic_block_ptrs_; }

private:
    Module *parent_;
    Type *return_type_;
    std::vector<std::unique_ptr<Argument>> arguments_;
    std::vector<Argument *> args_;
    std::vector<std::unique_ptr<BasicBlock>> basic_blocks_;
    std::vector<BasicBlock *> basic_block_ptrs_;
    bool is_instance_method_ = false;

    bool has_hidden_retval_ = false;
    Type *hidden_retval_type_ = nullptr;
};

//===----------------------------------------------------------------------===//
//                              Module
//===----------------------------------------------------------------------===//

class Module
{
    struct DoublePairHash
    {
        template <typename T1, typename T2>
        size_t operator()(const std::pair<T1, T2> &p) const
        {
            // ensure double is hashed fully
            static_assert(sizeof(double) == sizeof(uint64_t), "Unexpected double size");

            size_t hash1 = std::hash<T1>{}(p.first);
            uint64_t bit_width = 0;

            if constexpr (std::is_same_v<T2, double>)
            {
                // uint64_t convert double to uint64_t
                std::memcpy(&bit_width, &p.second, sizeof(double));
            }
            else
            {
                bit_width = std::hash<T2>{}(p.second);
            }

            size_t hash2 = std::hash<uint64_t>{}(bit_width);
            return hash1 ^ (hash2 << 1);
        }
    };

    struct DoublePairEqual
    {
        template <typename T1, typename T2>
        bool operator()(const std::pair<T1, T2> &lhs, const std::pair<T1, T2> &rhs) const
        {
            if (lhs.first != rhs.first)
                return false;

            if constexpr (std::is_same_v<T2, double>)
            {
                // 按二进制严格比较 double
                return std::memcmp(&lhs.second, &rhs.second, sizeof(double)) == 0;
            }
            else
            {
                return lhs.second == rhs.second;
            }
        }
    };

public:
    friend class VoidType;
    friend class PointerType;
    friend class StructType;
    friend class ArrayType;

    Module(std::string name = "");
    ~Module();

    Function *create_function(
        const std::string &name,
        Type *return_type,
        const std::vector<std::pair<std::string, Type *>> &params);

    Function *create_function(
        const std::string &name,
        Type *return_type,
        std::initializer_list<std::pair<std::string, Type *>> params)
    {
        return create_function(name, return_type, std::vector<std::pair<std::string, Type *>>(params));
    }

    Function *create_function(
        const std::string &name,
        FunctionType *type);

    GlobalVariable *create_global_variable(Type *type, bool is_constant, Constant *initializer, const std::string &name = "");

    Type *get_void_type();
    IntegerType *get_integer_type(uint8_t bit_width, bool unsigned_ = false);
    IntegerType *get_boolean_type();
    FloatType *get_float_type(uint8_t bit_width);

    PointerType *get_pointer_type(Type *element_type);
    FunctionType *get_function_type(Type *return_type, const std::vector<Type *> &param_types);

    ConstantInt *get_constant_int(IntegerType *type, uint64_t value);
    ConstantInt *get_constant_int(uint8_t bit_width, uint64_t value, bool unsigned_ = false);
    ConstantInt *get_constant_bool(bool value);

    ConstantFP *get_constant_fp(FloatType *type, double value);
    ConstantFP *get_constant_fp(uint8_t bit_width, double value);

    ConstantString *get_constant_string(std::string value);
    ConstantPointerNull *get_constant_pointer_null(PointerType *type);
    Constant *get_constant_zero(Type *type);
    ConstantAggregateZero *get_constant_aggregate_zero(Type *type);
    ConstantStruct *get_constant_struct(StructType *type, const std::vector<Constant *> &members);
    ConstantArray *get_constant_array(ArrayType *type, const std::vector<Constant *> &elements);

    const std::vector<Function *> functions() const
    {
        std::vector<Function *> result;
        result.reserve(functions_.size());
        for (auto &f : functions_)
            result.push_back(f.get());
        return result;
    }

    Function *get_function(const std::string &name) const
    {
        for (auto &f : functions_)
        {
            if (f->name() == name)
            {
                return f.get();
            }
        }
        return nullptr;
    }

    const std::vector<GlobalVariable *> global_variables() const
    {
        std::vector<GlobalVariable *> result;
        result.reserve(global_variables_.size());
        for (auto &gv : global_variables_)
            result.push_back(gv.get());
        return result;
    }
    
    std::vector<StructType *> struct_types() const
    {
        std::vector<StructType *> result;
        result.reserve(struct_types_.size());
        for (auto &s : struct_types_)
            result.push_back(s.get());
        return result;
    }

    StructType *get_struct_type_anonymous(const std::vector<MemberInfo> &members);
    StructType *try_get_named_global_type(const std::string &name);
    ArrayType *get_array_type(Type *element_type, uint64_t num_elements);
    StructType *get_struct_type(const std::string &name, const std::vector<MemberInfo> &members);
    VectorType *get_vector_type(Type *element_type, uint64_t num_elements);

private:
    std::string name_;
    std::unique_ptr<VoidType> void_type_;
    // (bit_width, unsigned) -> integer_type
    std::unordered_map<std::pair<uint8_t, bool>, std::unique_ptr<IntegerType>> integer_types_;
    std::unordered_map<uint8_t, std::unique_ptr<FloatType>> float_types_;
    // (element_type) -> pointer_type
    std::unordered_map<Type *, std::unique_ptr<PointerType>> pointer_types_;

    std::unordered_map<std::pair<Type *, uint64_t>,
                       std::unique_ptr<ConstantInt>>
        constant_ints_;

    std::unordered_map<
        std::pair<Type *, double>,
        std::unique_ptr<ConstantFP>,
        DoublePairHash,
        DoublePairEqual>
        constant_fps_;

    std::vector<std::unique_ptr<Function>> functions_;
    std::vector<std::unique_ptr<GlobalVariable>> global_variables_;

    std::vector<std::unique_ptr<ConstantStruct>> constant_structs_;
    std::vector<std::unique_ptr<ConstantArray>> constant_arrays_;
    std::vector<std::unique_ptr<ConstantString>> constant_strings_;
    std::vector<std::unique_ptr<ConstantPointerNull>> constant_pointer_nulls_;
    std::vector<std::unique_ptr<ConstantAggregateZero>> constant_aggregate_zeros_;

    friend class ArrayType;
    friend class StructType;

    // Type storage
    std::unordered_map<std::pair<Type *, uint64_t>,
                       std::unique_ptr<ArrayType>>
        array_types_;
    std::vector<std::unique_ptr<StructType>> struct_types_;
    std::unordered_map<std::pair<Type *, uint64_t>, std::unique_ptr<VectorType>> vector_types_;

    using FunctionTypeKey = std::pair<Type *, std::vector<Type *>>;
    struct FunctionTypeKeyHash
    {
        size_t operator()(const FunctionTypeKey &key) const
        {
            size_t hash = std::hash<Type *>{}(key.first);
            for (auto *type : key.second)
            {
                hash ^= std::hash<Type *>{}(type);
            }
            return hash;
        }
    };
    std::unordered_map<FunctionTypeKey, std::unique_ptr<FunctionType>, FunctionTypeKeyHash> function_types_;
};

//===----------------------------------------------------------------------===//
//                              Constant
//===----------------------------------------------------------------------===//
class Constant : public Value
{
public:
    // std::string as_string() const override = 0;
    virtual std::string as_string() const = 0;

protected:
    Constant(Type *type, const std::string &name = "")
        : Value(type, name) {}
};

// Integer constant
class ConstantInt : public Constant
{
public:
    uint64_t value() const { return value_; }

    ConstantInt *zext_value(Module *m, IntegerType *dest_type) const;
    ConstantInt *sext_value(Module *m, IntegerType *dest_type) const;

    std::string as_string() const override;

private:
    ConstantInt(IntegerType *type, uint64_t value);

    uint64_t value_;
    friend class Module;
};

// Floating-point constant
class ConstantFP : public Constant
{
public:
    double value() const { return value_; }

    std::string as_string() const override;

private:
    ConstantFP(FloatType *type, double value);

    double value_;
    friend class Module;
};

class ConstantAggregate : public Constant
{
public:
    const std::vector<Constant *> &elements() const { return elements_; }

    std::string as_string() const override;

protected:
    ConstantAggregate(Type *type, const std::vector<Constant *> &elements)
        : Constant(type), elements_(elements) {}

    std::vector<Constant *> elements_;
    friend class Module;
};

// Array constant
class ConstantArray : public ConstantAggregate
{
public:
    std::string as_string() const override;

private:
    ConstantArray(ArrayType *type, const std::vector<Constant *> &elements)
        : ConstantAggregate(type, elements) {}

    friend class Module;
};

// String constant
class ConstantString : public Constant
{
public:
    const std::string &value() const;
    std::string as_string() const override;

private:
    ConstantString(ArrayType *type, const std::string &value);
    static std::string escape_string(const std::string &input);

    std::string value_;
    friend class Module;
};

// Structure constant
class ConstantStruct : public ConstantAggregate
{
public:
    std::string as_string() const override;

private:
    ConstantStruct(StructType *type, const std::vector<Constant *> &elements)
        : ConstantAggregate(type, elements) {}

    friend class Module;
};

class GlobalVariable : public Constant
{
public:
    bool is_constant() const { return is_constant_; }
    Constant *initializer() const { return initializer_; }

    std::string as_string() const override;

private:
    GlobalVariable(Type *type, bool is_constant, Constant *initializer,
                   const std::string &name)
        : Constant(type, name), is_constant_(is_constant),
          initializer_(initializer) {}

    bool is_constant_;
    Constant *initializer_;
    friend class Module;
};

// Null pointer constant
class ConstantPointerNull : public Constant
{
public:
    std::string as_string() const override;

private:
    ConstantPointerNull(PointerType *type)
        : Constant(type) {}

    friend class Module;
};

// Aggregate zero constant
class ConstantAggregateZero : public Constant
{
public:
    std::string as_string() const override;

private:
    ConstantAggregateZero(Type *type)
        : Constant(type) {}

    friend class Module;
};

//===----------------------------------------------------------------------===//
//                           Instruction Subclasses
//===----------------------------------------------------------------------===//

class BinaryInst : public Instruction
{
public:
    static BinaryInst *create(Opcode op, Value *lhs, Value *rhs, BasicBlock *parent, const std::string &name = "");
    Value *left() const { return operand(0); }
    Value *right() const { return operand(1); }

protected:
    BinaryInst(Opcode op, Type *type, BasicBlock *parent, std::vector<Value *> operands, const std::string &name);

private:
    static bool is_binary_op(Opcode op);
};

class UnaryInst : public Instruction
{
public:
    static UnaryInst *create(Opcode op, Value *operand, BasicBlock *parent, const std::string &name = "");
    Value *get_operand() const { return operand(0); }

private:
    UnaryInst(Opcode op, Type *type, BasicBlock *parent, std::vector<Value *> operands, const std::string &name);
    static bool isUnaryOp(Opcode op);
};

class BranchInst : public Instruction
{
public:
    static BranchInst *create(BasicBlock *target, BasicBlock *parent);

    static BranchInst *create_cond(Value *cond, BasicBlock *true_bb,
                                   BasicBlock *false_bb, BasicBlock *parent);

    bool is_conditional() const
    {
        auto sz = operands_.size();
        MO_ASSERT(sz == 3 || sz == 1, "BranchInst should have 1 or 3 operands");
        return sz == 3;
    }
    BasicBlock *get_true_successor() const;
    BasicBlock *get_false_successor() const;

private:
    BranchInst(BasicBlock *target, BasicBlock *parent,
               std::vector<Value *> ops);

    BasicBlock *true_bb_;
    BasicBlock *false_bb_;
};

class ReturnInst : public Instruction
{
public:
    static ReturnInst *create(Value *value, BasicBlock *parent);

    Value *value() const { return operands_.size() > 0 ? operands_[0] : nullptr; }

private:
    ReturnInst(Value *value, BasicBlock *parent);
};

class UnreachableInst : public Instruction
{
public:
    static UnreachableInst *create(BasicBlock *parent);

private:
    UnreachableInst(BasicBlock *parent);
};

class PhiInst : public Instruction
{
public:
    static PhiInst *create(Type *type, BasicBlock *parent);

    void add_incoming(Value *val, BasicBlock *bb);

    unsigned num_incoming() const { return operands_.size() / 2; }
    Value *get_incoming_value(unsigned i) const { return operands_[2 * i]; }
    BasicBlock *get_incoming_block(unsigned i) const;

private:
    PhiInst(Type *type, BasicBlock *parent);
};

class ICmpInst : public BinaryInst
{
public:
    enum Predicate
    {
        EQ,
        NE,
        SLT,
        SLE,
        SGT,
        SGE,
        ULT,
        ULE,
        UGT,
        UGE
    };

    static ICmpInst *create(Predicate pred, Value *lhs, Value *rhs,
                            BasicBlock *parent);

    Predicate predicate() const { return pred_; }

private:
    ICmpInst(BasicBlock *parent, std::vector<Value *> ops);

    Predicate pred_;
};

class FCmpInst : public BinaryInst
{
public:
    enum Predicate
    {
        EQ,
        NE,
        LT,
        LE,
        GT,
        GE,
        ONE,
        OEQ,
        OLT,
        OLE,
        OGT,
        OGE
    };

    static FCmpInst *create(Predicate pred, Value *lhs, Value *rhs, BasicBlock *parent, const std::string &name = "");
    Predicate predicate() const { return pred_; }

private:
    FCmpInst(Predicate pred, BasicBlock *parent, std::vector<Value *> operands, const std::string &name);
    Predicate pred_;
};

//===----------------------------------------------------------------------===//
//                           Memory Operation Instruction Subclasses
//===----------------------------------------------------------------------===//
class AllocaInst : public Instruction
{
public:
    static AllocaInst *create(Type *allocated_type, BasicBlock *parent,
                              const std::string &name = "");

    Type *allocated_type() const { return allocated_type_; }

private:
    AllocaInst(Type *allocated_type, Type *ptr_type, BasicBlock *parent);

    Type *allocated_type_;
};

class LoadInst : public Instruction
{
public:
    static LoadInst *create(Value *ptr, BasicBlock *parent,
                            const std::string &name = "");

    Value *pointer() const { return operand(0); }

private:
    LoadInst(Type *loaded_type, BasicBlock *parent, Value *ptr);
};

class StoreInst : public Instruction
{
public:
    static StoreInst *create(Value *value, Value *ptr, BasicBlock *parent);

    // Stored value
    Value *value() const { return operand(0); }
    Value *pointer() const { return operand(1); }

private:
    StoreInst(BasicBlock *parent, Value *value, Value *ptr);
};

//===----------------------------------------------------------------------===//
//      Address Calculation Instruction Subclasses
//===----------------------------------------------------------------------===//
class GetElementPtrInst : public Instruction
{
public:
    static GetElementPtrInst *create(Value *ptr, std::vector<Value *> indices,
                                     BasicBlock *parent,
                                     const std::string &name = "");

    Value *base_pointer() const { return operand(0); }
    const std::vector<Value *> indices() const;

private:
    static Type *get_result_type(Type *base_type, const std::vector<Value *> &indices);

    GetElementPtrInst(Type *result_type, BasicBlock *parent,
                      Value *ptr, std::vector<Value *> indices);
};

// FIXME: Deprecated use CastInst instead
class ConversionInst : public Instruction
{
public:
    static ConversionInst *create(Opcode op, Value *val, Type *dest_type, BasicBlock *parent, const std::string &name = "");
    Value *get_source() const { return operand(0); }
    Type *get_dest_type() const { return type(); }

private:
    ConversionInst(Opcode op, Type *dest_type, BasicBlock *parent, std::vector<Value *> operands, const std::string &name);
    static bool isConversionOp(Opcode op);
};

class CastInst : public Instruction
{
protected:
    CastInst(Opcode op, Type *target_type, BasicBlock *parent,
             std::initializer_list<Value *> operands,
             const std::string &name);

public:
    Value *source() const { return operand(0); }
    Type *target_type() const { return type(); }
};

class BitCastInst : public CastInst
{
public:
    static BitCastInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    BitCastInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class PtrToIntInst : public CastInst
{
public:
    static PtrToIntInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    PtrToIntInst(BasicBlock *parent, Value *ptr, Type *target_type, const std::string &name) : CastInst(Opcode::PtrToInt, target_type, parent, {ptr}, name) {}
};

class CallInst : public Instruction
{
public:
    static CallInst *create(Value *callee, Type *return_type, const std::vector<Value *> &args, BasicBlock *parent, const std::string &name);
    static CallInst *create(Function *callee, const std::vector<Value *> &args, BasicBlock *parent, const std::string &name);

    // Warning: This function is not safe to use if the function is not a direct
    // function call, but rather a function pointer or a function reference.
    Function *called_function() const { return dynamic_cast<Function *>(operand(0)); }
    std::vector<Value *> create_operand_list(Value *callee, const std::vector<Value *> &args);
    std::vector<Value *> arguments() const;

private:
    CallInst(BasicBlock *parent, Value *callee, Type *return_type, const std::vector<Value *> &args, const std::string &name);
};

class RawCallInst : public Instruction
{
public:
    static RawCallInst *create(Value *callee, const std::vector<Value *> &args, BasicBlock *parent, const std::string &name);

    Value *callee() const { return operand(0); }
    std::vector<Value *> arguments() const;

private:
    RawCallInst(BasicBlock *parent, Value *callee, const std::vector<Value *> &args, const std::string &name);
};

class SExtInst : public CastInst
{
public:
    static SExtInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    SExtInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class ZExtInst : public CastInst
{
public:
    static ZExtInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    ZExtInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class TruncInst : public CastInst
{
public:
    static TruncInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    TruncInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class SIToFPInst : public CastInst
{
public:
    static SIToFPInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    SIToFPInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class FPToSIInst : public CastInst
{
public:
    static FPToSIInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    FPToSIInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class FPExtInst : public CastInst
{
public:
    static FPExtInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    FPExtInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class FPTruncInst : public CastInst
{
public:
    static FPTruncInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    FPTruncInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class IntToPtrInst : public CastInst
{
public:
    static IntToPtrInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    IntToPtrInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class FPToUIInst : public CastInst
{
public:
    static FPToUIInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    FPToUIInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

class UIToFPInst : public CastInst
{
public:
    static UIToFPInst *create(Value *val, Type *target_type, BasicBlock *parent, const std::string &name);

    Value *source_value() const { return operand(0); }
    Type *target_type() const { return type(); }

private:
    UIToFPInst(BasicBlock *parent, Value *val, Type *target_type, const std::string &name);
};

//===----------------------------------------------------------------------===//
//      Structure Layout
//===----------------------------------------------------------------------===//

struct Member
{
    Type *type;
    size_t offset;
};

struct StructLayout
{
    std::vector<Member> members;
    size_t size;
    size_t alignment;
};

StructLayout calculate_aligned_layout(const std::vector<Type *> &members);
