#include <cstdio>
#include <concepts>

using int32 = int;
using uint32 = unsigned int;
using uint64 = long long unsigned int;
using std::nullptr_t;

#define nassert(...)

namespace Nickel::System::Runtime::Alchemy
{
    struct TypeId
    {
        static constexpr uint32 Invalid = 0;
        static constexpr uint32 Type = 1;
        static constexpr uint32 Null = 2;
        static constexpr uint32 Bool = 3;
        static constexpr uint32 Int = 4;
        static constexpr uint32 UInt = 5;
        static constexpr uint32 Float = 6;
        static constexpr uint32 Double = 7;

        uint32 value;

        constexpr TypeId()
            : value( Invalid )
        {
        }

        constexpr TypeId( uint32 value )
            : value( value )
        {
        }

        explicit operator uint32() const { return value; }
        explicit operator bool() const { return value != 0; }

        static constexpr const char* getName( uint32 type_id )
        {
            switch( type_id )
            {
                case Type: return "type";
                case Null: return "null";
                case Bool: return "bool";
                case Int: return "int";
                case UInt: return "uint";
                case Float: return "float";
                case Double: return "double";
            }

            return "(invalid)";
        }
    };

    class Value
    {
        // tagged value layouts

        // short layout ( 32bit )
        // type: 0xffff0000 + 32bit type id
        // null: 0xffff0001 + 00000000
        // bool(true): 0xffff0002 + 00000001
        // bool(false): 0xffff0002 + 00000000
        // int: 0xffff0003 + 32bit payload
        // uint: 0xffff0004 + 32bit payload
        // float: 0xffff0005 + 32bit payload

        // reference layout ( 48bit )
        // reference: 0x0000 + 48bit payload(pointer)
        // reference include string, tuple, array, function, object, cfunction, cobject

        // long layout ( 64bit )
        // double: range( 0x0001, 0xfffe ) + ( native_double_value + 0x0001000000000000 );

        static constexpr uint64 LayoutMask = 0xffff000000000000;
        static constexpr uint64 ShortLayout = 0xffff000000000000;
        static constexpr uint64 ReferenceLayout = 0x0000000000000000;
        
        static constexpr uint64 LongValueTagMask = 0xffff000000000000;
        static constexpr uint32 ReferenceTag = 0x0000000000000000;

        static constexpr uint32 TypeIdTag = 0xffff0000;
        static constexpr uint32 NullTag = 0xffff0001;
        static constexpr uint32 BoolTag = 0xffff0002;
        static constexpr uint32 IntTag = 0xffff0003;
        static constexpr uint32 UIntTag = 0xffff0004;
        static constexpr uint32 FloatTag = 0xffff0005;
        
        static constexpr uint64 DoubleEncodingOffset = 0x0001000000000000;
        static constexpr uint64 ReferenceEncodingMask = 0x0000ffffffffffff;

    public:
        static constexpr uint64 InvalidType = TypeIdTag | TypeId::Invalid;
        static constexpr uint64 TypeType = TypeIdTag | TypeId::Type;
        static constexpr uint64 NullType = TypeIdTag | TypeId::Null;
        static constexpr uint64 BoolType = TypeIdTag | TypeId::Bool;
        static constexpr uint64 IntType = TypeIdTag | TypeId::Int;
        static constexpr uint64 UIntType = TypeIdTag | TypeId::UInt;
        static constexpr uint64 FloatType = TypeIdTag | TypeId::Float;
        static constexpr uint64 DoubleType = TypeIdTag | TypeId::Double;

        static constexpr uint64 NullValue = NullTag;
        static constexpr uint64 TrueValue = BoolTag | 0x00000001;
        static constexpr uint64 FalseValue = BoolTag | 0x00000000;

    private:
        union
        {
            uint64 data;

            struct
            {
                union
                {
                    TypeId type_id;
                    int32 int_value;
                    uint32 uint_value;
                    float float_value;
                } value;

                uint32 tag;

            } short_layout;

            struct
            {
                union
                {
                    double double_value;
                    uint64 double_data;
                } value;
            } double_layout;

            struct
            {
                void* reference_value;
            } reference_layout;
        };

        static inline constexpr Value encodeDouble( double value )
        {
            Value v;
            v.double_layout.value.double_value = value;
            v.double_layout.value.double_data += DoubleEncodingOffset;

            return v;
        }

        static inline constexpr double decodeDouble( Value value )
        {
            value.double_layout.value.double_data -= DoubleEncodingOffset;
            return value.double_layout.value.double_value;
        }

    public:
        constexpr Value()
            : data { NullValue }
        {
        }

        constexpr Value( uint64 data )
            : data( data )
        {
        }

        constexpr Value( TypeId value )
        {
            setTypeId( value );
        }

        constexpr Value( nullptr_t )
        {
            setNull();
        }

        constexpr Value( bool value )
        {
            setBool( value );
        }

        constexpr Value( int32 value )
        {
            setInt( value );
        }

        constexpr Value( uint32 value )
        {
            setUInt( value );
        }

        constexpr Value( float value )
        {
            setFloat( value );
        }

        constexpr Value( void* value )
        {
            setReference( value );
        }

        constexpr Value( double value )
        {
            setDouble( value );
        }

        explicit operator nullptr_t() const { nassert( isNull() ); return nullptr; }
        explicit operator bool() const { nassert( isBool() ); return getBool(); }
        //explicit operator int32() const { nassert( isInt() ); return getInt(); }
        explicit operator uint32() const { nassert( isUInt() ); return getUInt(); }
        explicit operator float() const { nassert( isFloat() ); return getFloat(); }
        explicit operator void*() const { nassert( isReference() ); return getReference(); }
        explicit operator double() const { nassert( isDouble() ); return getDouble(); }

        inline constexpr bool isShortLayout() const { return ( data & LayoutMask ) == ShortLayout; }
        inline constexpr bool isReferenceLayout() const { return ( data & LayoutMask ) == ReferenceLayout; }
        inline constexpr bool isDoubleLayout() const { return !isShortLayout() && !isReferenceLayout(); }

        inline constexpr bool isTypeId() const { return short_layout.tag == TypeIdTag; }
        inline constexpr void setTypeId( TypeId value ) { short_layout = { { .type_id = value }, TypeIdTag }; }
        inline constexpr TypeId getTypeId() const { return short_layout.value.type_id; }

        inline constexpr bool isNull() const { return short_layout.tag == NullTag; }
        inline constexpr void setNull() { data = NullValue; }
        inline constexpr nullptr_t getNull() { return nullptr; }

        inline constexpr bool isBool() const { return short_layout.tag == BoolTag; }
        inline constexpr void setBool( bool value ) { data = value ? TrueValue : FalseValue; }
        inline constexpr bool getBool() const { return data == TrueValue; }
        inline constexpr void setTrue() { data = TrueValue; }
        inline constexpr void setFalse() { data = FalseValue; }
        inline constexpr bool isTrue() const { return data == TrueValue; }
        inline constexpr bool isFalse() const { return data == FalseValue; }

        inline constexpr bool isInt() const { return short_layout.tag == IntTag; }
        inline constexpr void setInt( int32 value ) { short_layout = { { .int_value = value }, IntTag }; }
        inline constexpr int32 getInt() const { return short_layout.value.int_value; }

        inline constexpr bool isUInt() const { return short_layout.tag == UIntTag; }
        inline constexpr void setUInt( uint32 value ) { short_layout = { { .uint_value = value }, UIntTag }; }
        inline constexpr uint32 getUInt() const { return short_layout.value.uint_value; }

        inline constexpr bool isFloat() const { return short_layout.tag == FloatTag; }
        inline constexpr void setFloat( float value ) { short_layout = { { .float_value = value }, FloatTag }; }
        inline constexpr float getFloat() const { return short_layout.value.float_value; }

        inline constexpr bool isReference() const { return isReferenceLayout(); }
        inline constexpr void setReference( void* value ) { reference_layout.reference_value = value; }
        inline constexpr void* getReference() const { return reference_layout.reference_value; }

        inline constexpr bool isDouble() const { return isDoubleLayout(); }
        inline constexpr void setDouble( double value ) { *this = encodeDouble( value ); }
        inline constexpr double getDouble() const { return decodeDouble( *this ); }

        inline constexpr bool isNumeric() const { return isInt() || isUInt() || isFloat() || isDouble(); }
        inline constexpr bool isValid() const { return data != InvalidType; }

        inline constexpr TypeId getType() const
        {
            if( isShortLayout() )
            {
                switch( short_layout.tag )
                {
                    case TypeIdTag:
                        return TypeId::Type;
                    case NullTag:
                        return TypeId::Null;
                    case BoolTag:
                        return TypeId::Bool;
                    case IntTag:
                        return TypeId::Int;
                    case UIntTag:
                        return TypeId::UInt;
                    case FloatTag:
                        return TypeId::Float;
                    default:
                        return TypeId::Invalid;
                }
            }
            else if( isReferenceLayout() )
            {
                // TODO: implement abstract type deduction, do not support abstract value for now
                return TypeId::Invalid;
            }
            
            return TypeId::Double;
        }

        template<typename F>
        constexpr auto apply( F f )
        {
            if( isShortLayout() )
            {
                switch( short_layout.tag )
                {
                    case TypeIdTag:
                        return f( getTypeId() );
                    case NullTag:
                        return f( getNull() );
                    case IntTag:
                        return f( getInt() );
                    case UIntTag:
                        return f( getUInt() );
                    case FloatTag:
                        return f( getFloat() );
                    default:
                    {
                        nassert( false, "Value", "Invalid tag, value corruption detected" );
                        // return as if value is null
                        return f( getNull() );
                    }
                }
            }
            else if( isReferenceLayout() )
            {
                return f( getReference() );
            }

            return f( getDouble() );
        }
    };
}

using Value = ::Nickel::System::Runtime::Alchemy::Value;

template<std::size_t I>
struct Instruction;

template<>
struct Instruction<0>
{
    template<typename T>
    static constexpr bool Addable = std::same_as<T, int32> || std::same_as<T, uint32> || std::same_as<T, float> || std::same_as<T, double>;
    static inline constexpr Value evaluate1( Value lhs, Value rhs )
    {
        Value result = lhs.apply( [rhs]<typename T>( T lhs_value ) mutable {
            if constexpr( Addable<T> )
            {
                // workaround the bug that nested lambda can't capture parameters in parent scope
                T redirect_lhs_value = lhs_value;
                return rhs.apply( [redirect_lhs_value]<typename U>( U rhs_value ) {
                    if constexpr( Addable<U> )
                    {
                        return Value( redirect_lhs_value + rhs_value );
                    }
                    return Value( Value::InvalidType );
                } );
            }
            return Value( Value::InvalidType );
        } );
        return result;
    }

    static inline constexpr Value evaluate2( Value lhs, Value rhs )
    {
        if( lhs.isInt() )
        {
            if( rhs.isInt() )
            {
                return Value( lhs.getInt() + rhs.getInt() );
            }
            else if( rhs.isUInt() )
            {
                return Value( lhs.getInt() + rhs.getUInt() );
            }
            else if( rhs.isFloat() )
            {
                return Value( lhs.getInt() + rhs.getFloat() );
            }
            else if( rhs.isDouble() )
            {
                return Value( lhs.getInt() + rhs.getDouble() );
            }
        }
        else if( lhs.isUInt() )
        {
            if( lhs.isInt() )
            {
                if( rhs.isInt() )
                {
                    return Value( lhs.getUInt() + rhs.getInt() );
                }
                else if( rhs.isUInt() )
                {
                    return Value( lhs.getUInt() + rhs.getUInt() );
                }
                else if( rhs.isFloat() )
                {
                    return Value( lhs.getUInt() + rhs.getFloat() );
                }
                else if( rhs.isDouble() )
                {
                    return Value( lhs.getUInt() + rhs.getDouble() );
                }
            }
        }
        else if( lhs.isFloat() )
        {
            if( lhs.isInt() )
            {
                if( rhs.isInt() )
                {
                    return Value( lhs.getFloat() + rhs.getInt() );
                }
                else if( rhs.isUInt() )
                {
                    return Value( lhs.getFloat() + rhs.getUInt() );
                }
                else if( rhs.isFloat() )
                {
                    return Value( lhs.getFloat() + rhs.getFloat() );
                }
                else if( rhs.isDouble() )
                {
                    return Value( lhs.getFloat() + rhs.getDouble() );
                }
            }
        }
        else if( lhs.isDouble() )
        {
            if( lhs.isInt() )
            {
                if( rhs.isInt() )
                {
                    return Value( lhs.getDouble() + rhs.getInt() );
                }
                else if( rhs.isUInt() )
                {
                    return Value( lhs.getDouble() + rhs.getUInt() );
                }
                else if( rhs.isFloat() )
                {
                    return Value( lhs.getDouble() + rhs.getFloat() );
                }
                else if( rhs.isDouble() )
                {
                    return Value( lhs.getDouble() + rhs.getDouble() );
                }
            }
        }

        return Value::InvalidType;
    }
};

int main( int argc, char** argv )
{
    //int result = Instruction<0>::evaluate1( Value( 1 ), Value( argc ) ).getInt();
    int result = Instruction<0>::evaluate2( Value( 1 ), Value( argc ) ).getInt();

    return result;
}