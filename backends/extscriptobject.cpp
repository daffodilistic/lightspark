/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010  Timon Van Overveldt (timonvo@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <string>
#include <sstream>

#include "asobject.h"
#include "scripting/class.h"
#include "scripting/toplevel.h"
#include "compat.h"

#include "extscriptobject.h"

using namespace lightspark;
using namespace std;

/* -- ExtIdentifier -- */
// Constructors
ExtIdentifier::ExtIdentifier() :
	type(EI_STRING), strValue(""), intValue(0)
{
}
ExtIdentifier::ExtIdentifier(const std::string& value) :
	type(EI_STRING), strValue(value), intValue(0)
{
	stringToInt();
}
ExtIdentifier::ExtIdentifier(const char* value) :
	type(EI_STRING), strValue(value), intValue(0)
{
	stringToInt();
}

ExtIdentifier::ExtIdentifier(int32_t value) :
	type(EI_INT32), strValue(""), intValue(value)
{
}
ExtIdentifier::ExtIdentifier(const ExtIdentifier& other)
{
	type = other.getType();
	strValue = other.getString();
	intValue = other.getInt();
}

// Convert integer string identifiers to integer identifiers
void ExtIdentifier::stringToInt()
{
	intValue = atoi(strValue.c_str());
	std::stringstream conv;
	conv << intValue;

	if(conv.str() == strValue)
		type = EI_INT32;
}

// Comparator
bool ExtIdentifier::operator<(const ExtIdentifier& other) const
{
	if(getType() == EI_STRING && other.getType() == EI_STRING)
		return getString() < other.getString();
	else if(getType() == EI_INT32 && other.getType() == EI_INT32)
		return getInt() < other.getInt();
	else if(getType() == EI_INT32 && other.getType() == EI_STRING)
		return true;
	return false;
}

/* -- ExtObject -- */
// Constructors
ExtObject::ExtObject() : type(EO_OBJECT)
{
}
ExtObject::ExtObject(const ExtObject& other)
{
	type = other.getType();
	other.copy(properties);
}

// Copying
ExtObject& ExtObject::operator=(const ExtObject& other)
{
	type = other.getType();
	other.copy(properties);
	return *this;
} 
void ExtObject::copy(std::map<ExtIdentifier, ExtVariant>& dest) const
{
	dest = properties;
}

// Properties
bool ExtObject::hasProperty(const ExtIdentifier& id) const
{
	return properties.find(id) != properties.end();
}
ExtVariant* ExtObject::getProperty(const ExtIdentifier& id) const
{
	std::map<ExtIdentifier, ExtVariant>::const_iterator it = properties.find(id);
	if(it == properties.end())
		return NULL;

	return new ExtVariant(it->second);
}
void ExtObject::setProperty(const ExtIdentifier& id, const ExtVariant& value)
{
	properties[id] = value;
}
bool ExtObject::removeProperty(const ExtIdentifier& id)
{
	std::map<ExtIdentifier, ExtVariant>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
}
bool ExtObject::enumerate(ExtIdentifier*** ids, uint32_t* count) const
{
	*count = properties.size();
	*ids = new ExtIdentifier*[properties.size()];
	std::map<ExtIdentifier, ExtVariant>::const_iterator it;
	int i = 0;
	for(it = properties.begin(); it != properties.end(); ++it)
	{
		(*ids)[i] = new ExtIdentifier(it->first);
		i++;
	}

	return true;
}

/* -- ExtVariant -- */
// Constructors
ExtVariant::ExtVariant() :
	type(EV_VOID), strValue(""), intValue(0), doubleValue(0), booleanValue(false)
{
}
ExtVariant::ExtVariant(const std::string& value) :
	type(EV_STRING), strValue(value), intValue(0), doubleValue(0), booleanValue(false)
{
}
ExtVariant::ExtVariant(const char* value) :
	type(EV_STRING), strValue(value), intValue(0), doubleValue(0), booleanValue(false)
{
}
ExtVariant::ExtVariant(int32_t value) :
	type(EV_INT32), strValue(""), intValue(value), doubleValue(0), booleanValue(false)
{
}
ExtVariant::ExtVariant(double value) :
	type(EV_DOUBLE), strValue(""), intValue(0), doubleValue(value), booleanValue(false)
{
}
ExtVariant::ExtVariant(bool value) :
	type(EV_BOOLEAN), strValue(""), intValue(0), doubleValue(0), booleanValue(value)
{
}
ExtVariant::ExtVariant(const ExtVariant& other)
{
	type = other.getType();
	strValue = other.getString();
	intValue = other.getInt();
	doubleValue = other.getDouble();
	booleanValue = other.getBoolean();
	objectValue = *other.getObject();
}
ExtVariant::ExtVariant(ASObject* other) :
	strValue(""), intValue(0), doubleValue(0), booleanValue(false)
{
	switch(other->getObjectType())
	{
	case T_STRING:
		strValue = other->toString().raw_buf();
		type = EV_STRING;
		break;
	case T_INTEGER:
		intValue = other->toInt();
		type = EV_INT32;
		break;
	case T_NUMBER:
		doubleValue = other->toNumber();
		type = EV_DOUBLE;
		break;
	case T_BOOLEAN:
		booleanValue = Boolean_concrete(other);
		type = EV_BOOLEAN;
		break;
	case T_ARRAY:
		objectValue.setType(ExtObject::EO_ARRAY);
	case T_OBJECT:
		type = EV_OBJECT;
		{
			bool hasNext = false;
			ASObject* nextName = NULL;
			ASObject* nextValue = NULL;
			unsigned int index = 0;
			while(other->hasNext(index, hasNext) && hasNext)
			{
				other->nextName(index, nextName);
				other->nextValue(index-1, nextValue);

				if(nextName->getObjectType() == T_INTEGER)
					objectValue.setProperty(nextName->toInt(), nextValue);
				else
					objectValue.setProperty(nextName->toString().raw_buf(), nextValue);

				nextName->decRef();
				nextValue->decRef();
			}
		}
		break;
	case T_NULL:
		type = EV_NULL;
		break;
	case T_UNDEFINED:
	default:
		type = EV_VOID;
		break;
	}
}

// Conversion to ASObject
ASObject* ExtVariant::getASObject() const
{
	ASObject* asobj;
	switch(getType())
	{
	case EV_STRING:
		asobj = Class<ASString>::getInstanceS(getString().c_str());
		break;
	case EV_INT32:
		asobj = abstract_i(getInt());
		break;
	case EV_DOUBLE:
		asobj = abstract_d(getDouble());
		break;
	case EV_BOOLEAN:
		asobj = abstract_b(getBoolean());
		break;
	case EV_OBJECT:
		{
			ExtObject* objValue = getObject();

			ExtVariant* property;
			uint32_t count;

			// We are converting an array, so lets set indexes
			if(objValue->getType() == ExtObject::EO_ARRAY)
			{
				asobj = Class<Array>::getInstanceS();
				count = objValue->getLength();
				static_cast<Array*>(asobj)->resize(count);
				for(uint32_t i = 0; i < count; i++)
				{
					property = objValue->getProperty(i);
					static_cast<Array*>(asobj)->set(i, property->getASObject());
					delete property;
				}
			}
			// We are converting an object, so lets set variables
			else
			{
				asobj = Class<ASObject>::getInstanceS();
			
				ExtIdentifier** ids;
				uint32_t count;
				std::stringstream conv;
				if(objValue != NULL && objValue->enumerate(&ids, &count))
				{
					for(uint32_t i = 0; i < count; i++)
					{
						property = objValue->getProperty(*ids[i]);

						if(ids[i]->getType() == ExtIdentifier::EI_STRING)
							asobj->setVariableByQName(ids[i]->getString(), "", property->getASObject());
						else
						{
							conv << ids[i]->getInt();
							asobj->setVariableByQName(conv.str().c_str(), "", property->getASObject());
						}
						delete property;
						delete ids[i];
					}
				}
				delete ids;
			}
			if(objValue != NULL)
				delete objValue;
		}
		break;
	case EV_NULL:
		asobj = new Null;
		break;
	case EV_VOID:
	default:
		asobj = new Undefined;
		break;
	}
	asobj->incRef();
	return asobj;
}

/* -- ExtCallbackFunction -- */
bool ExtCallbackFunction::operator()(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, ExtVariant** result)
{
	if(iFunc != NULL)
	{
		// Convert raw arguments to objects
		ASObject* objArgs[argc];
		for(uint32_t i = 0; i < argc; i++)
		{
			objArgs[i] = args[i]->getASObject();
		}

		ASObject* objResult = iFunc->call(new Null, objArgs, argc);
		if(objResult != NULL)
			*result = new ExtVariant(objResult);
		return true;
	}
	else
		return extFunc(so, id, args, argc, result);
}