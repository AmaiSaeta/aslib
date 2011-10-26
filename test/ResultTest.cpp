#include <iostream>
#include <typeinfo>
#include <boost/test/auto_unit_test.hpp>

std::ostream &operator<<(std::ostream &stream, const std::type_info &type);

#include <aslib/Result.hpp>
using aslib::Result;

namespace {
	enum CallFlag {
		construct,
		copyConstruct,
		moveConstruct,
		copyOpEp,
		moveOpEp,
		destruct
	};
	struct SucceessTypeTag {};
	struct FailedTypeTag {};
	template<typename kindTag>
	struct CoType {
		CoType() : flag_(nullptr) {}
		CoType(CallFlag *flag) : flag_(flag) { *flag_ = construct; }
		CoType(const CoType &src) : flag_(src.flag_) { if(flag_) *flag_ = copyConstruct; }
		CoType(CoType &&src) : flag_(src.flag_) { if(flag_) *flag_ = moveConstruct; }
		CoType &operator=(const CoType &rhs) { flag_ = rhs.flag_; if(flag_) *flag_ = copyOpEp; return *this; }
		CoType &operator=(CoType &&rhs) { flag_ = rhs.flag_; if(flag_) *flag_ = moveOpEp; return *this; }
		~CoType() { if(flag_) *flag_ = destruct; }
		CallFlag *flag_;
	};
	typedef CoType<SucceessTypeTag> SucceededType;
	typedef CoType<FailedTypeTag> FailedType;

	typedef Result<SucceededType, FailedType> R;

	struct Fixture {
		Fixture()
			: s(SucceededType(&s_flag)), f(FailedType(&f_flag)),
			  cs(SucceededType(&cs_flag)), cf(FailedType(&cf_flag)) {}
		R u,s,f;	// (non-const) uninitialized, in-succeeded, in-failed
		const R cu, cs, cf;	// (const) uninitialized, in-succeeded, in-failed
		CallFlag u_flag, s_flag, f_flag, cu_flag, cs_flag, cf_flag;
	};
}

BOOST_FIXTURE_TEST_SUITE(ResultTest, Fixture)
	BOOST_AUTO_TEST_CASE(testConstructor) {
			SucceededType s;
			FailedType f;
			R ru, rs(s), rf(f);
			const R cru, crs(s), crf(f);
	}

	BOOST_AUTO_TEST_CASE(testCopyConstructor) {
		R sd(s), fd(f), ud(u);
		BOOST_CHECK(static_cast<bool>(sd));
		BOOST_CHECK(!static_cast<bool>(fd));
		BOOST_CHECK(!static_cast<bool>(ud));
		BOOST_CHECK_THROW(ud.fail(), R::NotHaveFailedObject);

		const R csd(s), cfd(f), cud(u);
		BOOST_CHECK(static_cast<bool>(csd));
		BOOST_CHECK(!static_cast<bool>(cfd));
		BOOST_CHECK(!static_cast<bool>(cud));
		BOOST_CHECK_THROW(cud.fail(), R::NotHaveFailedObject);
	}

	BOOST_AUTO_TEST_CASE(testMoveConstructor) {
		CallFlag s_flag, f_flag;
		SucceededType st(&s_flag);
		FailedType ft(&f_flag);
		BOOST_REQUIRE_EQUAL(s_flag, construct);
		BOOST_REQUIRE_EQUAL(f_flag, construct);

		R rs(std::move(st));
		R rf(std::move(ft));
		BOOST_CHECK_EQUAL(s_flag, moveConstruct);
		BOOST_CHECK_EQUAL(f_flag, moveConstruct);
	}

	BOOST_AUTO_TEST_CASE(testMoveConstructor_from_Result) {
		// from Result(succeeded)
		R sDst(std::move(s));
		BOOST_CHECK_EQUAL(s_flag, moveConstruct);
		// from Result(failed)
		R fDst(std::move(f));
		BOOST_CHECK_EQUAL(f_flag, moveConstruct);

		// 未初期化のResultからのmove constructはチェックのしようがない……
	}

	BOOST_AUTO_TEST_CASE(testDestructor) {
		CallFlag sflag, fflag;
		{
			SucceededType st(&sflag);
			FailedType ft(&fflag);
			R rs(st), rf(ft);
			BOOST_REQUIRE_NE(sflag, destruct);
			BOOST_REQUIRE_NE(fflag, destruct);
		}
		BOOST_CHECK_MESSAGE(sflag==destruct, "not destruct rs");
		BOOST_CHECK_MESSAGE(fflag==destruct, "not destruct rf");
	}

	BOOST_AUTO_TEST_CASE(testOpBool) {
		BOOST_CHECK(static_cast<bool>(s));
		BOOST_CHECK(!static_cast<bool>(f));
		BOOST_CHECK(!static_cast<bool>(u));

		BOOST_CHECK(static_cast<bool>(cs));
		BOOST_CHECK(!static_cast<bool>(cf));
		BOOST_CHECK(!static_cast<bool>(cu));
	}

	BOOST_AUTO_TEST_CASE(testOpDereference) {	// operator *
		BOOST_CHECK_EQUAL(typeid(*s), typeid(SucceededType&));
		BOOST_CHECK_THROW(*f, R::CantDereference);
		BOOST_CHECK_THROW(*u, R::CantDereference);

		BOOST_CHECK_EQUAL(typeid(*cs), typeid(const SucceededType&));
		BOOST_CHECK_THROW(*cf, R::CantDereference);
		BOOST_CHECK_THROW(*cu, R::CantDereference);
	}

	BOOST_AUTO_TEST_CASE(testOpArrow) {	// operator->
		BOOST_CHECK_EQUAL(typeid(s.operator->()), typeid(SucceededType*));
		BOOST_CHECK_THROW(f.operator->(), R::CantDereference);
		BOOST_CHECK_THROW(u.operator->(), R::CantDereference);

		BOOST_CHECK_EQUAL(typeid(cs.operator->()), typeid(const SucceededType*));
		BOOST_CHECK_THROW(cf.operator->(), R::CantDereference);
		BOOST_CHECK_THROW(cu.operator->(), R::CantDereference);
	}

	BOOST_AUTO_TEST_CASE(testFail) {
		BOOST_CHECK_THROW(s.fail(), R::NotHaveFailedObject);
		BOOST_CHECK_EQUAL(typeid(f.fail()), typeid(FailedType&));
		BOOST_CHECK_THROW(u.fail(), R::NotHaveFailedObject);

		BOOST_CHECK_THROW(cs.fail(), R::NotHaveFailedObject);
		BOOST_CHECK_EQUAL(typeid(cf.fail()), typeid(const FailedType&));
		BOOST_CHECK_THROW(cu.fail(), R::NotHaveFailedObject);
	}

	BOOST_AUTO_TEST_CASE(testOpEp) {	// operator=
		R r1, r2;
		BOOST_REQUIRE(!static_cast<bool>(r1));
		BOOST_REQUIRE(!static_cast<bool>(r2));

		r1 = r2 = s;
		BOOST_CHECK(static_cast<bool>(r1));
		BOOST_CHECK(static_cast<bool>(r2));

		r1 = r2 = f;
		BOOST_CHECK(!static_cast<bool>(r1));
		BOOST_CHECK_NO_THROW(r1.fail());
		BOOST_CHECK(!static_cast<bool>(r2));
		BOOST_CHECK_NO_THROW(r2.fail());

		r1 = r2 = u;
		BOOST_CHECK(!static_cast<bool>(r1));
		BOOST_CHECK_THROW(r1.fail(), R::NotHaveFailedObject);
		BOOST_CHECK(!static_cast<bool>(r2));
		BOOST_CHECK_THROW(r2.fail(), R::NotHaveFailedObject);
	}

/*
 Result::operator=(TYPE&&)用テストケース
 TEST_CASE_OPEP_MOVE_(左辺値型種別, 右辺値型種別)
   uninit	未初期化のResult
   succeeded	正常値のResult
   failed	エラー値のResult
   co-succeeded	正常値
   co-failed	エラー値
*/
#define TEST_CASE_OPEP_MOVE_(lhs, rhs) \
	BOOST_AUTO_TEST_CASE(testOpEp_move_ ## lhs ## _ ## rhs) { \
		TEST_CASE_OPEP_MOVE_LHS_ ## lhs; \
		TEST_CASE_OPEP_MOVE_RHS_ ## rhs; \
	}
#define TEST_CASE_OPEP_MOVE_LHS_uninit    R r;
#define TEST_CASE_OPEP_MOVE_LHS_succeeded SucceededType lst; R r(lst);
#define TEST_CASE_OPEP_MOVE_LHS_failed    FailedType lft; R r(lft);

#define TEST_CASE_OPEP_MOVE_RHS_uninit BOOST_FAIL("Can't runned test, because flag is not exist.");
#define TEST_CASE_OPEP_MOVE_RHS_succeeded \
	r = std::move(s); \
	BOOST_CHECK_EQUAL(s_flag, moveOpEp);
#define TEST_CASE_OPEP_MOVE_RHS_failed \
	r = std::move(f); \
	BOOST_CHECK_EQUAL(f_flag, moveOpEp);
#define TEST_CASE_OPEP_MOVE_RHS_co_uninit TEST_CASE_OPEP_MOVE_RHS_uninit
#define TEST_CASE_OPEP_MOVE_RHS_co_succeeded \
	CallFlag flag; \
	SucceededType rst(&flag); \
	r = std::move(rst); \
	BOOST_CHECK_EQUAL(flag, moveOpEp);
#define TEST_CASE_OPEP_MOVE_RHS_co_failed \
	CallFlag flag; \
	FailedType rft(&flag); \
	r = std::move(rft); \
	BOOST_CHECK_EQUAL(flag, moveOpEp);

	TEST_CASE_OPEP_MOVE_(uninit, succeeded);
	TEST_CASE_OPEP_MOVE_(uninit, failed);
	TEST_CASE_OPEP_MOVE_(uninit, co_succeeded);
	TEST_CASE_OPEP_MOVE_(uninit, co_failed);
	TEST_CASE_OPEP_MOVE_(succeeded, succeeded);
	TEST_CASE_OPEP_MOVE_(succeeded, failed);
	TEST_CASE_OPEP_MOVE_(succeeded, co_succeeded);
	TEST_CASE_OPEP_MOVE_(succeeded, co_failed);
	TEST_CASE_OPEP_MOVE_(failed, succeeded);
	TEST_CASE_OPEP_MOVE_(failed, failed);
	TEST_CASE_OPEP_MOVE_(failed, co_succeeded);
	TEST_CASE_OPEP_MOVE_(failed, co_failed);
	
#undef TEST_CASE_OPEP_MOVE_
#undef TEST_CASE_OPEP_MOVE_LHS_uninit
#undef TEST_CASE_OPEP_MOVE_LHS_succeeded
#undef TEST_CASE_OPEP_MOVE_LHS_failed
#undef TEST_CASE_OPEP_MOVE_RHS_uninit
#undef TEST_CASE_OPEP_MOVE_RHS_succeeded
#undef TEST_CASE_OPEP_MOVE_RHS_failed
#undef TEST_CASE_OPEP_MOVE_RHS_co_uninit
#undef TEST_CASE_OPEP_MOVE_RHS_co_succeeded
#undef TEST_CASE_OPEP_MOVE_RHS_co_failed

	BOOST_AUTO_TEST_CASE(testCoTypedefs) {
		typedef Result<SucceededType, FailedType> R;
		BOOST_CHECK_EQUAL(typeid(R::SucceededType), typeid(SucceededType));
		BOOST_CHECK_EQUAL(typeid(R::FailedType), typeid(FailedType));
	}
BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(ResultCantDereferenceTest)
	BOOST_AUTO_TEST_CASE(test) {
		R::CantDereference e;	// 構築テスト
		std::runtime_error &s = R::CantDereference();	// 基底クラス確認
		e.what();	// 呼び出しテスト
	}
BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(ResultNotHaveFailedObjectTest)
	BOOST_AUTO_TEST_CASE(test) {
		R::NotHaveFailedObject e;	// 構築テスト
		std::runtime_error &s = R::NotHaveFailedObject();	// 基底クラス確認
		e.what();	// 呼び出しテスト
	}
BOOST_AUTO_TEST_SUITE_END();
