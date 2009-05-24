#pragma once
#include "rpc_common.hpp"
#include <hash_map>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/back.hpp>

#include <boost/mpl/identity.hpp>

#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/function_types/function_arity.hpp>

namespace rpc{

    namespace detail{
        /**
        * 本类用以从某输入archive中反序列化参数列表,
        * 并以该参数列表调用指定的函数、获取返回值。
        * 本类为虚基类。参数列表个数、类型皆由其子类决定。
        */
        class UnserializerAndCaller : private boost::noncopyable{
        public:
            /**
            * 从ar中解析出参数列表，并执行函数，返回值序列化到ret串中。
            * @param iar_params 参数输入流。从中获得函数的各参数。
            * @param oar_result 接收运算返回值的输出流。
            */
            virtual void unserializeAndCall ( iarchive& iar_params, oarchive& oar_result ) = 0;
            /**
            * 虚析构函数。
            */
            virtual ~UnserializerAndCaller(){}
        };

        template <class RetType>
        class UnserializerAndCaller0: public UnserializerAndCaller{
        public:
            typedef RPC_FUNCTION_CLASS<RetType(void)> func_type;
        protected:
            func_type func;
        public:
            UnserializerAndCaller0(const func_type& fnc):func(fnc)
            {}

            virtual void unserializeAndCall( iarchive& iar_params, oarchive& oar_result )
            {
                RetType retVal = func();
				oar_result << packet( false, remote_call_error(), std::string() );
                oar_result << retVal;
            }
        };

    #define UAC_BEGIN \
        template <class RetType,

    #define UAC_PROTO(i) >\
        class UnserializerAndCaller ## i: public UnserializerAndCaller{\
        public:\
        typedef RPC_FUNCTION_CLASS<RetType(

    #define UAC_BODY(i) )> func_type;\
        protected:\
        func_type func;\
        public:\
        UnserializerAndCaller ## i(const func_type& fnc):func(fnc)\
        {}\
        \
        virtual void unserializeAndCall( iarchive& iar_params, oarchive& oar_result )\
        {

    #define UAC_END \
			oar_result << packet( false, remote_call_error(), std::string() );\
			oar_result << retVal;\
        }\
        };

    #define UAC_iar(i) T ## i t ## i;  iar_params>>t ## i;

        //UnserializerAndCaller1:
        UAC_BEGIN class T1 UAC_PROTO(1) const T1& UAC_BODY(1)
        UAC_iar(1)
            RetType retVal = func(t1);
        UAC_END;

        //UnserializerAndCaller2:
        UAC_BEGIN class T1, class T2 
		UAC_PROTO(2) const T1&, const T2& UAC_BODY(2)
        UAC_iar(1) UAC_iar(2)
            RetType retVal = func(t1, t2);
        UAC_END;

        //UnserializerAndCaller3:
        UAC_BEGIN class T1, class T2, class T3 
		UAC_PROTO(3) const T1&, const T2&, const T3& UAC_BODY(3)
        UAC_iar(1) UAC_iar(2) UAC_iar(3)
            RetType retVal = func(t1, t2, t3);
        UAC_END;

        //UnserializerAndCaller4:
        UAC_BEGIN class T1, class T2, class T3, class T4
		UAC_PROTO(4) const T1&, const T2&, const T3&, const T4& UAC_BODY(4)
        UAC_iar(1) UAC_iar(2) UAC_iar(3) UAC_iar(4)
            RetType retVal = func(t1, t2, t3, t4);
        UAC_END;

        //UnserializerAndCaller5:
        UAC_BEGIN class T1, class T2, class T3, class T4, class T5
		UAC_PROTO(5) const T1&, const T2&, const T3&, const T4&, const T5& UAC_BODY(5)
        UAC_iar(1) UAC_iar(2) UAC_iar(3) UAC_iar(4) UAC_iar(5)
            RetType retVal = func(t1, t2, t3, t4, t5);
        UAC_END;

		// RetType=void, template specialization

        template <>
        class UnserializerAndCaller0<void>: public UnserializerAndCaller{
        public:
            typedef RPC_FUNCTION_CLASS<void(void)> func_type;
        protected:
            func_type func;
        public:
            UnserializerAndCaller0(const func_type& fnc):func(fnc)
            {}

            virtual void unserializeAndCall( iarchive& iar_params, oarchive& oar_result )
            {
                func();
				oar_result << packet( false, remote_call_error(), std::string() );\
            }
        };

    #define void_UAC_BEGIN \
        template <

    #define void_UAC_PROTO(i) >\
        class UnserializerAndCaller ## i < void, 
		
	#define void_UAC_PROTO_NEXT >\
		: public UnserializerAndCaller{\
        public:\
        typedef RPC_FUNCTION_CLASS< void (

    #define void_UAC_BODY(i) )> func_type;\
        protected:\
        func_type func;\
        public:\
        UnserializerAndCaller ## i(const func_type& fnc):func(fnc)\
        {}\
        \
        virtual void unserializeAndCall( iarchive& iar_params, oarchive& oar_result )\
        {

    #define void_UAC_END \
			oar_result << packet( false, remote_call_error(), std::string() );\
        }\
        };

    #define void_UAC_iar(i) T ## i t ## i;  iar_params>>t ## i;

        //UnserializerAndCaller1:
        void_UAC_BEGIN		class T1
		void_UAC_PROTO(1)	T1
		void_UAC_PROTO_NEXT	const T1&
		void_UAC_BODY(1)
			void_UAC_iar(1)
            func(t1);
        void_UAC_END;

        //UnserializerAndCaller2:
        void_UAC_BEGIN		class T1, class T2
		void_UAC_PROTO(2)	T1, T2
		void_UAC_PROTO_NEXT	const T1&, const T2&
		void_UAC_BODY(2)
			void_UAC_iar(1) void_UAC_iar(2)
            func(t1, t2);
        void_UAC_END;

        //UnserializerAndCaller3:
        void_UAC_BEGIN		class T1, class T2, class T3
		void_UAC_PROTO(3)	T1, T2, T3
		void_UAC_PROTO_NEXT	const T1&, const T2&, const T3&
		void_UAC_BODY(3)
			void_UAC_iar(1) void_UAC_iar(2) void_UAC_iar(3)
            func(t1, t2, t3);
        void_UAC_END;

        //UnserializerAndCaller4:
        void_UAC_BEGIN		class T1, class T2, class T3, class T4
		void_UAC_PROTO(4)	T1, T2, T3, T4
		void_UAC_PROTO_NEXT	const T1&, const T2&, const T3&, const T4&
		void_UAC_BODY(4)
			void_UAC_iar(1) void_UAC_iar(2) void_UAC_iar(3) void_UAC_iar(4)
            func(t1, t2, t3, t4);
        void_UAC_END;

        //UnserializerAndCaller5:
        void_UAC_BEGIN		class T1, class T2, class T3, class T4, class T5
		void_UAC_PROTO(5)	T1, T2, T3, T4, T5
		void_UAC_PROTO_NEXT	const T1&, const T2&, const T3&, const T4&, const T5&
		void_UAC_BODY(5)
			void_UAC_iar(1) void_UAC_iar(2) void_UAC_iar(3) void_UAC_iar(4) void_UAC_iar(5)
            func(t1, t2, t3, t4, t5);
        void_UAC_END;

        typedef RPC_SHARED_PTR<detail::UnserializerAndCaller> UnserializerAndCallerPtr;
    }


    /**
     * 函数集。
     */
    class function_set : private boost::noncopyable{
        typedef stdext::hash_map<std::string, detail::UnserializerAndCallerPtr> map_type;
        map_type id2funcmap;
        boost::recursive_mutex mtx;
    public:
        function_set(){}

        /**
         * 往函数集中新增一个函数。
         * @param funcId 用来标识此函数的id。8位字符串类型。
         * @param func 函数。
         * @exception func_id_already_exists 集中已存在一个相同名称的函数。
         */
        template<class FuncType>
        void add(const std::string& func_id, FuncType* func)
            throw(func_id_already_exists)
        {
            add<FuncType>(func_id, RPC_FUNCTION_CLASS<FuncType>(func));
        }

        /**
        * 往函数集中新增一个函数。
        * @param func_id 用来标识此函数的id。8位字符串类型。
        * @param func 函数。
        * @exception func_id_already_exists 集中已存在一个相同名称的函数。
        */
        template<class Signature>
        void add(const std::string& func_id,
            const typename RPC_FUNCTION_CLASS<Signature>& func)
        {
            using namespace boost;
            typedef function_types::parameter_types<Signature>::type ArgTypes;
            typename registerFunctionImplStruct<Signature, function_types::function_arity<Signature>::value> obj;
            obj.invoke(func_id, func, this);
        }

        /**
        * 从函数集中移除一个函数。
        * @param func_id 用来标识此函数的id。8位字符串类型。
        * @exception func_id_not_found 集中不存在此名称的RPC函数。
        */
        void remove(std::string func_id) throw(func_id_not_found)
        {
            boost::recursive_mutex::scoped_lock lck(mtx);
            map_type::const_iterator itr;
            if ( (itr=id2funcmap.find(func_id)) != id2funcmap.end() )
            {
                id2funcmap.erase(itr);
            }else{
                throw func_id_not_found();
            }
        }


        /**
        * 执行一个RPC函数。
        * @param func_id 用来标识此函数的id。8位字符串类型。
        * @exception func_id_not_found 集中不存在此名称的RPC函数。
        */
        void invoke(const std::string& func_id, iarchive& iar_params, oarchive& oar_result )
            throw(func_id_not_found, boost::archive::archive_exception)
        {
            boost::recursive_mutex::scoped_lock lck(mtx);
            map_type::const_iterator itr;
            if ( (itr=id2funcmap.find(func_id)) != id2funcmap.end() )
            {
				try
				{
					(*itr).second->unserializeAndCall(iar_params, oar_result);
				}
				catch( const boost::archive::archive_exception& e )
				{
					oar_result << packet( false, remote_call_error(remote_call_error::func_call_error,
						"Invalid parameter: \""+std::string( e.what() )+"\"."), std::string() );
				}
            }
			else
			{
                oar_result << packet( false, remote_call_error(remote_call_error::func_not_found,
					"RPC function \""+func_id+"\" not found."), std::string() );
            }
        }
    private:
#pragma region registerFunctionImpl functions

        template<class Signature, int ArgCount>
        struct registerFunctionImplStruct{
        };

        template<class Signature>
        struct registerFunctionImplStruct<Signature, 0>
        {
            void invoke(const std::string& funcId, const typename RPC_FUNCTION_CLASS<Signature>& func, function_set* ths)
                throw(func_id_already_exists)
            {
                using namespace boost;
                ths->registerFunctionImpl<function_types::result_type<Signature>::type>(funcId, func);
            }
        };

        template<class Signature>
        struct registerFunctionImplStruct<Signature, 1>
        {
            void invoke(const std::string& funcId, const typename RPC_FUNCTION_CLASS<Signature>& func, function_set* ths)
                throw(func_id_already_exists)
            {
                using namespace boost;
                typedef function_types::parameter_types<Signature>::type ArgTypes;
                ths->registerFunctionImpl<function_types::result_type<Signature>::type, boost::mpl::at_c<ArgTypes, 0>::type>(funcId, func);
            }
        };

        template<class Signature>
        struct registerFunctionImplStruct<Signature, 2>
        {
            void invoke(const std::string& funcId, const typename RPC_FUNCTION_CLASS<Signature>& func, function_set* ths)
                throw(func_id_already_exists)
            {
                using namespace boost;
                typedef function_types::parameter_types<Signature>::type ArgTypes;
                ths->registerFunctionImpl<function_types::result_type<Signature>::type,
                    boost::mpl::at_c<ArgTypes, 0>::type, boost::mpl::at_c<ArgTypes, 1>::type>(funcId, func);
            }
        };

        template<class Signature>
        struct registerFunctionImplStruct<Signature, 3>
        {
            void invoke(const std::string& funcId, const typename RPC_FUNCTION_CLASS<Signature>& func, function_set* ths)
                throw(func_id_already_exists)
            {
                using namespace boost;
                typedef function_types::parameter_types<Signature>::type ArgTypes;
                ths->registerFunctionImpl<function_types::result_type<Signature>::type,
                    boost::mpl::at_c<ArgTypes, 0>::type, boost::mpl::at_c<ArgTypes, 1>::type,
                    boost::mpl::at_c<ArgTypes, 2>::type>(funcId, func);
            }
        };

        template<class Signature>
        struct registerFunctionImplStruct<Signature, 4>
        {
            void invoke(const std::string& funcId, const typename RPC_FUNCTION_CLASS<Signature>& func, function_set* ths)
                throw(func_id_already_exists)
            {
                using namespace boost;
                typedef function_types::parameter_types<Signature>::type ArgTypes;
                ths->registerFunctionImpl<function_types::result_type<Signature>::type,
                    boost::mpl::at_c<ArgTypes, 0>::type, boost::mpl::at_c<ArgTypes, 1>::type,
                    boost::mpl::at_c<ArgTypes, 2>::type, boost::mpl::at_c<ArgTypes, 3>::type>(funcId, func);
            }
        };

        template<class Signature>
        struct registerFunctionImplStruct<Signature, 5>
        {
            void invoke(const std::string& funcId, const typename RPC_FUNCTION_CLASS<Signature>& func, function_set* ths)
                throw(func_id_already_exists)
            {
                using namespace boost;
                typedef function_types::parameter_types<Signature>::type ArgTypes;
                ths->registerFunctionImpl<function_types::result_type<Signature>::type,
                    boost::mpl::at_c<ArgTypes, 0>::type, boost::mpl::at_c<ArgTypes, 1>::type,
                    boost::mpl::at_c<ArgTypes, 2>::type, boost::mpl::at_c<ArgTypes, 3>::type,
                    boost::mpl::at_c<ArgTypes, 4>::type>(funcId, func);
            }
        };

#pragma endregion

        template<class RetType>
        void registerFunctionImpl(const std::string& funcId, const typename detail::UnserializerAndCaller0<RetType>::func_type& func)
            throw(func_id_already_exists)
        {
            boost::recursive_mutex::scoped_lock lck(mtx);
            map_type::iterator itr ;
            if( (itr=this->id2funcmap.find(funcId)) != this->id2funcmap.end() ){
                throw func_id_already_exists();
            }else{
                this->id2funcmap[funcId] = detail::UnserializerAndCallerPtr(new detail::UnserializerAndCaller0<RetType>(func));
            }
        }

#define registerFunction_BEGIN \
    template<class RetType, 
#define registerFunction_PROTO(i) >\
    void registerFunctionImpl(const std::string& funcId,\
    const typename detail::UnserializerAndCaller ## i<RetType,
#define registerFunction_BODY(i) >::func_type& func)\
    throw(func_id_already_exists)\
    {\
		boost::recursive_mutex::scoped_lock lck(mtx);\
        map_type::iterator itr ;\
		if( (itr=this->id2funcmap.find(funcId)) != this->id2funcmap.end() ){\
			throw func_id_already_exists();\
		}else{\
			this->id2funcmap[funcId] = detail::UnserializerAndCallerPtr(new detail::UnserializerAndCaller ## i<RetType,
#define registerFunction_END >(func));\
		}\
    }\

        //registerFunction: 1 param
        registerFunction_BEGIN class T1 registerFunction_PROTO(1) T1 registerFunction_BODY(1)
            T1 registerFunction_END;

        //registerFunction: 2 param
        registerFunction_BEGIN class T1, class T2 registerFunction_PROTO(2) T1, T2 registerFunction_BODY(2)
            T1, T2 registerFunction_END;

        //registerFunction: 3 param
        registerFunction_BEGIN class T1, class T2, class T3 registerFunction_PROTO(3) T1, T2, T3 registerFunction_BODY(3)
            T1, T2, T3 registerFunction_END;

        //registerFunction: 4 param
        registerFunction_BEGIN class T1, class T2, class T3, class T4 registerFunction_PROTO(4) T1, T2, T3, T4 registerFunction_BODY(4)
            T1, T2, T3, T4 registerFunction_END;

        //registerFunction: 5 param
        registerFunction_BEGIN class T1, class T2, class T3, class T4, class T5 registerFunction_PROTO(5) T1, T2, T3, T4, T5 registerFunction_BODY(5)
            T1, T2, T3, T4, T5 registerFunction_END;
	};
	typedef RPC_SHARED_PTR<function_set> function_set_ptr;
}
