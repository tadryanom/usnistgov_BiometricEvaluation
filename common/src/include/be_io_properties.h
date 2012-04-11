/******************************************************************************
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 ******************************************************************************/
#ifndef __BE_IO_PROPERTIES_H__
#define __BE_IO_PROPERTIES_H__

#include <map>
#include <string>

#include <be_error_exception.h>
#include <be_io.h>
#include <be_memory_autoarray.h>

using namespace std;

namespace BiometricEvaluation
{
	namespace IO
	{
		/** Internal structure used for storing property keys/values */
		typedef map<string, string> PropertiesMap;

		/**
		* @brief
		* Maintain key/value pairs of strings, with each property
		* matched to one value.
		*/
		class Properties {
		public:
			/** Convenience const iterator over a Properties */
			typedef PropertiesMap::const_iterator Properties_iter;
			
			/**
			 * @brief
			 * Construct a new Properties object.
			 *
			 * @param[in] mode
			 * 	The read/write mode of the object.
			 *
			 * @return
			 *      An object representing the properties set.
			 */
			Properties(
			    uint8_t mode = IO::READWRITE);
			
			/**
			 * @brief
			 * Construct a new Properties object from the contents
			 * of a buffer.
			 * @details
			 * The format of the buffer can be seen in
			 * PropertiesFile.
			 *
			 * @param[in] buffer
			 *	A buffer that contains the contents of a 
			 *	Property file.
			 * @param[in] size
			 * 	The size of buffer.
			 *
			 * @return
			 *      An object representing the properties set.
			 *
			 * @throw Error::StrategyError
			 *	A line in the properties file is malformed.
			 */
			Properties(
			    const uint8_t *buffer,
			    const size_t size,
			    uint8_t mode = IO::READWRITE)
			    throw (Error::StrategyError);
			    
			/**
			 * @brief
			 * Set a property with a value.
			 * @details
			 * Both the property and value will have leading and
			 * trailing whitespace removed. If the property already
			 * exists in the set, its value will be replaced with
			 * the new value; otherwise, the property will be
			 * created.
			 *
			 * @param[in] property
			 *	The name of the property to set.
			 * @param[in] value
			 *	The value associated with the property.
			 *
			 * @throw Error::StrategyError
			 *	The Properties object is read-only.
			 */
			virtual void
			setProperty(
			    const string &property,
			    const string &value)
			    throw (Error::StrategyError);

			/**
			 * @brief
			 * Set a property with an integer value.
			 * @details
			 * The property will have leading and trailing
			 * whitespace removed.  If the property already exists
			 * in the set, its value will be replaced with the new
			 * value; otherwise the property will be created.
			 *
			 * @param[in] property
			 *	The name of the property to set.
			 * @param[in] value
			 *	The value associated with the property.
			 *
			 * @throw Error::StrategyError
			 *	The Properties object is read-only.
			 */
			virtual void
			setPropertyFromInteger(
			    const string &property,
			    int64_t value)
			    throw (Error::StrategyError);
			
			/**
			 * @brief
			 * Set a property with a double value.
			 * @details
			 * The property will have leading and trailing
			 * whitespace removed.  If the property already exists
			 * in the set, its value will be replaced with the new
			 * value; otherwise the property will be created.
			 *
			 * @param[in] property
			 *	The name of the property to set.
			 * @param[in] value
			 *	The value associated with the property.
			 *
			 * @throw Error::StrategyError
			 *	The Properties object is read-only.
			 */
			virtual void
			setPropertyFromDouble(
			    const string &property,
			    double value)
			    throw (Error::StrategyError);

			/**
			 * @brief
			 * Remove a property.
			 *
			 * @param[in] property
			 *	The name of the property to set.
			 *
			 * @throw Error::ObjectDoesNotExist
			 *	The named property does not exist.
			 * @throw Error::StrategyError
			 *	The Properties object is read-only.
			 */
			virtual void
			removeProperty(
			    const string &property)
			    throw (Error::ObjectDoesNotExist,
			    Error::StrategyError);

			/**
			 * @brief
			 * Retrieve a property value as a string object.
			 *
			 * @param[in] property
			 *	The name of the property to get.
			 *
			 * @throw Error::ObjectDoesNotExist
			 *	The named property does not exist.
			 */
			virtual string
			getProperty(
			    const string &property)
			    const
			    throw (Error::ObjectDoesNotExist);

			/**
			 * @brief
			 * Retrieve a property value as an integer value.
			 * @details
			 * Integer value strings for properties can represent
			 * either decimal or hexadecimal values, which must be
			 * preceded with either "0x" or "0X".
			 *
			 * @param[in] property
			 *	The name of the property to get.
			 *
			 * @throw Error::ObjectDoesNotExist
			 *	The named property does not exist.
			 * @throw Error::ConversionError
			 *	The property value cannot be converted, usually
			 *	 due to non-numeric characters in the string.
			 */
			virtual int64_t
			getPropertyAsInteger(
			    const string &property)
			    const
			    throw (Error::ObjectDoesNotExist,
			    Error::ConversionError);
				   
			/**
			 * @brief
			 * Retrieve a property value as a double value.
			 *
			 * @param[in] property
			 *	The name of the property to get.
			 *
			 * @throw Error::ObjectDoesNotExist
			 *	The named property does not exist.
			 */
			virtual double
			getPropertyAsDouble(
			    const string &property)
			    const
			    throw (Error::ObjectDoesNotExist);
			
			/** Destructor */
			virtual ~Properties();

		protected:
			/**
			 * @brief
			 * Obtain the mode of the Properties object.
			 *
			 * @return
			 *	Mode (IO::READONLY or IO::READWRITE)
			 */
			uint8_t
			getMode()
			    const;
			    
			/**
			 * @brief
			 * Initialize the PropertiesMap with the contents
			 * of a properly formatted buffer.
			 * @details
			 * This method ensures that the PropertiesMap contains
			 * only the properties found within the buffer.
			 *
			 * @param buffer
			 *	Contents of a properties file.
			 *
 			 * @throw Error::StrategyError
			 *	A line of the buffer is malformed.
			 */
			void
			initWithBuffer(
			    const Memory::uint8Array &buffer)
			    throw (Error::StrategyError);
			    
			/**
			 * @brief
			 * Initialize the PropertiesMap with the contents
			 * of a properly formatted buffer.
			 * @details
			 * This method ensures that the PropertiesMap contains
			 * only the properties found within the buffer.
			 *
			 * @param buffer
			 *	Contents of a properties file.
			 * @param size
			 *	Size of the buffer.
			 *
 			 * @throw Error::StrategyError
			 *	A line of the buffer is malformed.
			 */
			void
			initWithBuffer(
			    const uint8_t *const buffer,
			    size_t size)
			    throw (Error::StrategyError);
			
			/**
			 * @brief
			 * Obtain iterator to the first property.
			 *
			 * @return
			 *	Iterator to first property.
			 */
			Properties_iter
			begin()
			    const;
			
			/**
			 * @brief
			 * Obtain iterator to one past the last property.
			 *
			 * @return
			 *	Iterator one past the last property.
			 */
			Properties_iter
			end()
			    const;

		private:
			/** The map containing the property/value pairs */
			PropertiesMap _properties;
			
			/** Mode in which the Properties object was opened */
			uint8_t _mode;
		};
	}
}
#endif	/* __BE_IO_PROPERTIES_H__ */
