/*
  ==============================================================================

	hnode_DynamicType.h
	Created: 2 Sep 2018 6:48:30pm
	Author:  Christoph

  ==============================================================================
*/

#pragma once

namespace snex
{

#define CALC_IF_FLOAT(op) if(expectedType == Types::ID::Float && getType() == Types::ID::Float) data.f.value op (float)other;
#define CONVERT_TO_DOUBLE_WITH_OP(op) { data.d.type = Types::Double; data.d.value op other.toDouble(); }
#define CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(op) { CALC_IF_FLOAT(op) else CONVERT_TO_DOUBLE_WITH_OP(op ) }


class ObjectTypeRegister
{
public:

	int registerType(const Identifier& id)
	{
		auto index = getTypeIndex(id);

		if (index != -1)
			return index;

		entries.add(id);
		return entries.size() - 1;
	}

	int getTypeIndex(const Identifier& id) const
	{
		return entries.indexOf(id);
	}

private:

	Array<Identifier> entries;
};

class VariableStorage
{
public:

	VariableStorage();

	VariableStorage(const VariableStorage& other) :
		data(other.data)
	{};

	VariableStorage(Types::ID type_, const var& value);
	VariableStorage(FloatType s);
	VariableStorage(double d);
	VariableStorage(int s);
	VariableStorage(const Types::FloatBlock& b);
	VariableStorage(HiseEvent& m);

	VariableStorage(int objectType, void* objectPointer, bool isReallyAPointer);

	VariableStorage& operator =(int s);
	VariableStorage& operator =(FloatType s);
	VariableStorage& operator =(double s);
	VariableStorage& operator =(const Types::FloatBlock& s);

	bool operator==(const VariableStorage& other) const;

	void setWithType(Types::ID newType, double value);
	void set(FloatType s);
	void set(double s);
	void setDouble(double newValue);
	void set(int s);;
	void set(block&& s);;
	void set(block& b);
	void set(const HiseEvent& e);
	void set(void* objectPointer, const ObjectTypeRegister& objectRegister, const Identifier& typeId);

	void clear();

	explicit operator FloatType() const noexcept;
	explicit operator double() const noexcept;
	explicit operator int() const;
	explicit operator Types::FloatBlock() const;
	explicit operator HiseEvent() const;

	template <Types::ID expectedType> VariableStorage& store(const VariableStorage& other)
	{
		IF_CONSTEXPR (expectedType == Types::ID::Integer)
			data.i.value = (int)other;
		else IF_CONSTEXPR (expectedType == Types::ID::Float || expectedType == Types::ID::Double)
			CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(= )
		
		return *this;
	}
	

	template <Types::ID expectedType> VariableStorage& add(const VariableStorage& other)
	{
		IF_CONSTEXPR (expectedType == Types::ID::Integer)
			data.i.value += (int)other;
		else IF_CONSTEXPR (expectedType == Types::ID::Float || expectedType == Types::ID::Double)
			CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(+=)
		else IF_CONSTEXPR (expectedType == Types::ID::Block)
			data.b + (block)other;
		else IF_CONSTEXPR (expectedType == Types::ID::Signal)
		{
			if (other.getTypeValue() == Types::ID::Block)
				data.b + (block)other;
			else
				data.b + (FloatType)other;
		}

		return *this;
	}

	template <Types::ID expectedType> VariableStorage& and_(const VariableStorage& other)
	{
		IF_CONSTEXPR (expectedType == Types::ID::Integer)
			data.i.value &= (int)other;
		
		return *this;
	}

	template <Types::ID expectedType> VariableStorage& or_(const VariableStorage& other)
	{
		IF_CONSTEXPR (expectedType == Types::ID::Integer)
			data.i.value |= (int)other;

		return *this;
	}

	template <Types::ID expectedType> VariableStorage& mod(const VariableStorage& other)
	{
		IF_CONSTEXPR (expectedType == Types::ID::Integer)
			data.i.value %= (int)other;

		return *this;
	}

	template <Types::ID expectedType> VariableStorage& sub(const VariableStorage& other)
	{
		IF_CONSTEXPR (expectedType == Types::ID::Integer)
			data.i.value -= (int)other;
		else IF_CONSTEXPR (expectedType == Types::ID::Float || expectedType == Types::ID::Double)
			CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(-=)
		else IF_CONSTEXPR (expectedType == Types::ID::Block)
			data.b - (block)other;
		else IF_CONSTEXPR (expectedType == Types::ID::Signal)
		{
			if (other.getTypeValue() == Types::ID::Block)
				data.b - (block)other;
			else
				data.b - (FloatType)other;
		}

		return *this;
	}

	template <Types::ID expectedType> VariableStorage& div(const VariableStorage& other)
	{
		IF_CONSTEXPR (expectedType == Types::ID::Integer)
			data.i.value /= (int)other;
		else IF_CONSTEXPR (expectedType == Types::ID::Float || expectedType == Types::ID::Double)
			CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(/=)
		else IF_CONSTEXPR (expectedType == Types::ID::Signal)
		{
			if (other.getTypeValue() == Types::ID::Double)
				data.b * (FloatType)(1.0 / (double)other);
		}

		return *this;
	}

	template <Types::ID expectedType> VariableStorage& mul(const VariableStorage& other)
	{
		IF_CONSTEXPR (expectedType == Types::ID::Integer)
			data.i.value *= (int)other;
		else IF_CONSTEXPR (expectedType == Types::ID::Float || expectedType == Types::ID::Double)
			CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(*=)
		else IF_CONSTEXPR (expectedType == Types::ID::Block)
			data.b * (block)other;
		else IF_CONSTEXPR (expectedType == Types::ID::Signal)
		{
			if (other.getTypeValue() == Types::ID::Block)
				data.b * other.data.b;
			else
				data.b * (FloatType)other;
		}

		return *this;
	}

	Types::ID getType() const noexcept 
	{
		auto t = getTypeValue();
		
		if (t == 0)
			return Types::ID::Void;
		else if (t < (int)HiseEvent::Type::numTypes)
			return Types::ID::Event;
		else
			return (Types::ID)t;
	}

	double toDouble() const;
	FloatType toFloat() const;
	int toInt() const;
	block toBlock() const;
	HiseEvent toEvent() const;
	bool isVoid() const noexcept { return getType() == Types::ID::Void; }
	size_t getSizeInBytes() const noexcept;

	void* getObjectPointer(const ObjectTypeRegister& objectRegister, const Identifier& typeId) const;

	template <Types::ID TypeID> auto toType() const
	{
		IF_CONSTEXPR (TypeID == Types::ID::Float || TypeID == Types::ID::Float)
			return toFloat();
		else IF_CONSTEXPR (TypeID == Types::ID::Integer)
			return toInt();
		else IF_CONSTEXPR (TypeID == Types::ID::Block)
			return toBlock();
		else IF_CONSTEXPR (TypeID == Types::ID::Event)
			return toEvent();
		
		return 0;
	}

	void* getDataPointer()
	{
		if (getTypeValue() == Types::ID::Pointer)
			return data.p.data;
		if (getTypeValue() == Types::ID::Float ||
			getTypeValue() == Types::ID::Double ||
			getTypeValue() == Types::ID::Integer)
		{
			return &data.d.value;
		}
		else
			return &data;
	}

	int getPointerType() const;

private:
	 
	uint8 getTypeValue() const
	{
		return reinterpret_cast<const uint8*>(&data)[0];
	}

	struct DoubleData
	{
		int type;
		int unused;
		double value;
	};

	struct IntData
	{
		int type;
		int unused;
		int64 value;
	};

	struct FloatData
	{
		int type;
		int unused;
		float value;
		int unused2;
	};

	struct PointerData
	{
		int type;
		int dataType;
		void* data;
	};

	union Data
	{
		Data()
		{
			e = HiseEvent();
		}

		Data(const Data& other) :
			d(other.d)
		{};

		
		block b;
		DoubleData d;
		IntData i;
		FloatData f;
		HiseEvent e;
		PointerData p;
	};

	Data data;

#if 0
	double doubleValue;
	HiseEvent m;
	int intValue;
	block blockValue;

	Types::ID type;
#endif
};






}
