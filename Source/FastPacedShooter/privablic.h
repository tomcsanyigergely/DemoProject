#ifndef INCLUDED_PRIVABLIC_H
#define INCLUDED_PRIVABLIC_H

namespace privablic 
{  
	// Generate a static data member of type Stub::type in which to store
	// the address of a private member.  It is crucial that Stub does not
	// depend on the /value/ of the the stored address in any way so that
	// we can access it from ordinary code without directly touching
	// private data.
	template <class Stub>
	struct member
	{
		static typename Stub::type value;
	}; 
	template <class Stub> 
	typename Stub::type member<Stub>::value;

	// Generate a static data member whose constructor initializes
	// member<Stub>::value. This type will only be named in an explicit
	// instantiation, where it is legal to pass the address of a private
	// member.
	template <class Stub, typename Stub::type x>
	struct private_member
	{
		private_member() { member<Stub>::value = x; }
		static private_member instance;
	};
	template <class Stub, typename Stub::type x> 
	private_member<Stub, x> private_member<Stub, x>::instance;
}
#endif