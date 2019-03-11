/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <benjamin.viguier@epitech.eu> wrote this file. As long as you retain this 
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in
 * return Benjamin Viguier
 * --------------------------------------------------------------------------
 */

#pragma once

#include <unordered_map>
#include <vector>
#include <ostream>
#include <istream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <functional>

#if defined _MSC_VER
 #define STRCASECMP ::_stricmp
 #define STRDUP ::_strdup
#else
 #define STRCASECMP ::strcasecmp
 #define STRDUP ::strdup
#endif

namespace json
{
	class AbstractData;
	class Entity;
	using ObjEntry = std::pair<const std::string, Entity>;
	using makeObject = std::initializer_list<ObjEntry>;

	class Entity
	{
	public:
		enum CloneOption {
			LAZY,
			DEEP
		};
		enum Type {
			NBR,
			STR,
			ARR,
			OBJ,
			BOL,
			NUL
		};
		enum StringifyAttr {
			PRETTY,
			MINIFIED
		};

		using OnConflitFunc = std::function<void (std::string const &, Entity &, Entity&)>;
	private:
		static void
			_basicOnConfit(std::string const &key, Entity &first, Entity &second);
	public:
		Entity(Entity const &) = default;
		Entity(Type type = NUL);
		Entity(double nbr);
		Entity(long nbr);
		Entity(unsigned long nbr);
		Entity(int nbr);
		Entity(unsigned int nbr);
		Entity(bool bol);
		Entity(std::string const &str);
		Entity(const char *s);
		Entity(std::initializer_list<Entity> list);
		Entity(std::initializer_list<ObjEntry> list);

		Entity	&operator=(Entity const &other) = default;
		bool	operator==(Entity const &other) const;
		bool	operator!=(Entity const &other) const;

		Entity	&operator[](std::string const &key);
		Entity	&operator[](unsigned idx);
		Entity	&push(Entity const &obj);
		void	merge(Entity const &toAdd, OnConflitFunc const &onConflit = _basicOnConfit);
		
		Entity		clone(CloneOption attr = CloneOption::DEEP) const;
		std::string	stringify(StringifyAttr attr, int depth = 0) const;
		void		print(std::ostream &to, StringifyAttr attr, int depth = 0) const;

		bool	isObject(void);
		bool	isArray(void);
		bool	isNumber(void);
		bool	isString(void);
		bool	isBool(void);
		bool	isNull(void);

		template<typename T>
		T	to() const;
		
		template<typename T>
		T	&getData() {
			return *(reinterpret_cast<T*>(_data.get()));
		}

		template<typename T>
		const T	&getData() const {
			return *(reinterpret_cast<T*>(_data.get()));
		}

		template<typename T>
		auto	&value() {
			return (reinterpret_cast<T*>(_data.get()))->get();
		}

		template<typename T>
		const auto	&value() const {
			return (reinterpret_cast<T*>(_data.get()))->get();
		}

		template<typename T, typename U>
		U	value() {
			return (reinterpret_cast<T*>(_data.get()))->get();
		}

		template<typename T, typename U>
		U	value() const {
			return (reinterpret_cast<T*>(_data.get()))->get();
		}

		template<typename T>
		const T	&constGetData() const
		{
			return *(reinterpret_cast<T*>(_data.get()));
		}

		static inline Entity	newObject()
		{
			return Entity(OBJ);
		}

		static inline Entity	newArray()
		{
			return Entity(ARR);
		}
	private:
		Entity(std::shared_ptr<json::AbstractData> &data);
	private:
		std::shared_ptr<AbstractData>	_data;
	};
	std::ostream	&operator<<(std::ostream &to, Entity const &me);

	template <>
	int		Entity::to<int>() const;
	template <>
	long		Entity::to<long>() const;
	template <>
	double		Entity::to<double>() const;
	template <>
	std::string	Entity::to<std::string>() const;
	template <>
	char		*Entity::to<char *>() const;
	template <>
	bool		Entity::to<bool>() const;

	class AbstractData
	{
	public:
		virtual bool		operator==(AbstractData const &) const = 0;
		virtual Entity::Type	getType() const = 0;
		virtual std::shared_ptr<AbstractData>
		clone(Entity::CloneOption attr) const = 0;
		virtual	void
		print(std::ostream &stm, Entity::StringifyAttr attr, int depth) const = 0;

		virtual	std::string	toString(void) const;
		virtual double		toNumber(void) const;
		virtual bool		toBool(void) const;
	};

	class Number final : public AbstractData
	{
	public:
		Number(double nbr);
		
		virtual bool		operator==(AbstractData const &) const final;
		virtual Entity::Type	getType() const final;
		virtual std::shared_ptr<AbstractData>
		clone(Entity::CloneOption attr) const final;
		virtual	void
		print(std::ostream &stm, Entity::StringifyAttr attr, int depth) const final;
		double	get(void) const;
		void	set(double val);

		virtual	std::string	toString(void) const final;
		virtual double		toNumber(void) const final;
		virtual bool		toBool(void) const final;
	private:
		double	_value;
	};

	class Null final : public AbstractData
	{
	public:
		Null();
		
		virtual bool		operator==(AbstractData const &) const final;
		virtual Entity::Type	getType() const final;
		virtual std::shared_ptr<AbstractData>
		clone(Entity::CloneOption attr) const final;
		virtual	void
		print(std::ostream &stm, Entity::StringifyAttr attr, int depth) const final;
		void	*get(void) const;

		virtual	std::string	toString(void) const final;
	};

	class String final : public AbstractData, private std::string
	{
	public:
		String(std::string const &str);

		virtual bool		operator==(AbstractData const &) const final;
		virtual Entity::Type	getType() const final;
		virtual std::shared_ptr<AbstractData>
		clone(Entity::CloneOption attr) const final;
		virtual	void
		print(std::ostream &stm, Entity::StringifyAttr attr, int depth) const final;
		const std::string	&get(void) const;
        std::string         &get(void);
		void				set(std::string const &str);

		virtual	std::string	toString(void) const final;
		virtual double		toNumber(void) const final;
		virtual bool		toBool(void) const final;
	};

	class Bool final : public AbstractData
	{
	public:
		Bool(bool val);
		
		virtual bool		operator==(AbstractData const &) const final;
		virtual Entity::Type	getType() const final;
		virtual std::shared_ptr<AbstractData>
		clone(Entity::CloneOption attr) const final;
		virtual	void
		print(std::ostream &stm, Entity::StringifyAttr attr, int depth) const final;
		bool	get(void) const;
		void	set(bool bol);

		virtual	std::string	toString(void) const final;
		virtual double		toNumber(void) const final;
		virtual bool		toBool(void) const final;
	private:
		bool	_value;
	};

	class Object final : public AbstractData, private std::unordered_map<std::string, Entity>
	{
	public:
		Object();
		Object(std::initializer_list<ObjEntry> list);

		virtual bool		operator==(AbstractData const &) const final;
		virtual Entity::Type	getType() const final;
		virtual std::shared_ptr<AbstractData>
		clone(Entity::CloneOption attr) const final;
		virtual	void
		print(std::ostream &stm, Entity::StringifyAttr attr, int depth) const final;
		std::unordered_map<std::string, Entity>			&get(void);
		const std::unordered_map<std::string, Entity>	&get(void) const;

		virtual	std::string	toString(void) const final;
	};

	class Array final : public AbstractData, private std::vector<Entity>
	{
	public:
		Array();
		Array(std::initializer_list<Entity> list);

		virtual bool		operator==(AbstractData const &) const final;
		virtual Entity::Type	getType(void) const final;
		virtual std::shared_ptr<AbstractData>
		clone(Entity::CloneOption attr) const final;
		virtual	void
		print(std::ostream &stm, Entity::StringifyAttr attr, int depth) const final;
		std::vector<Entity>			&get(void);
		const std::vector<Entity>	&get(void) const;

		virtual	std::string	toString(void) const final;
	};
}
