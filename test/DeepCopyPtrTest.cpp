#include <boost/test/unit_test.hpp>

#include <aslib/DeepCopyPtr.hpp>

using aslib::DeepCopyPtr;

namespace {
	// テストで使用するユーザー定義クラス群

	struct C {
		C() : i_(0) {}
		C(int i) : i_(i) {}
		bool operator==(const C &rhs) const { return i_ == rhs.i_; }
		int i_;
	};
	struct D : C {
		D() {}
		D(int i) : C(i) {}
		D(const D &src) : C(src.i_) {}
	};

	std::ostream &operator<<(std::ostream &stream, const C &src)
	{
		stream << "[[" << typeid(src).name() << "::i_ = " << src.i_ << "]]";
		return stream;
	}
}

BOOST_AUTO_TEST_SUITE(DeepCopyPtrTest)

BOOST_AUTO_TEST_CASE(testConstructor) {
	DeepCopyPtr<int> p;
	BOOST_CHECK_EQUAL(p.get(), static_cast<int *>(0));

	C *c = new C();
	DeepCopyPtr<C> cp(c);
	BOOST_CHECK_EQUAL(cp.get(), c);

	D *d = new D();
	DeepCopyPtr<C> dp(d);
	BOOST_CHECK_EQUAL(dp.get(), d);

	DeepCopyPtr<C> cp0(static_cast<C *>(0));
	BOOST_CHECK_EQUAL(cp0.get(), static_cast<C *>(0));
}

BOOST_AUTO_TEST_CASE(testCopyConstructor) {
	DeepCopyPtr<D> dp1(new D());

	DeepCopyPtr<D> dp2(dp1);
	BOOST_CHECK_NE(dp2.get(), dp1.get());

	DeepCopyPtr<D> dp0;
	DeepCopyPtr<D> dp3(dp0);
	BOOST_CHECK_EQUAL(dp3.get(), static_cast<D *>(0));

	DeepCopyPtr<C> cp(dp1);
	BOOST_CHECK_NE(cp.get(), dp1.get());
}

BOOST_AUTO_TEST_CASE(testCopyConstructor_fromConstPtr) {
	const C *cnp = new C();
	DeepCopyPtr<C> cdp(cnp);
	BOOST_CHECK_EQUAL(*cnp, *cdp);
	BOOST_CHECK_NE(cnp, cdp.get());
	delete cnp;
}

BOOST_AUTO_TEST_CASE(testSwap) {
	DeepCopyPtr<C> p1(new C(1)), p2(new C(2));
	p1.swap(p2);
	BOOST_CHECK_EQUAL(p1->i_, 2);
	BOOST_CHECK_EQUAL(p2->i_, 1);
}

BOOST_AUTO_TEST_CASE(testReset_4sameType) {
	DeepCopyPtr<C> p0, p1(new C());

	// NULL→NULL
	p0.reset();
	BOOST_CHECK(!p0);

	// NULL→非NULL
	p0.reset(new C());
	BOOST_CHECK(p0);

	// 非NULL→NULL
	p1.reset(0);
	BOOST_CHECK(!p1);
}

BOOST_AUTO_TEST_CASE(testRset_4diffType) {
	DeepCopyPtr<C> cdp(new C());
	C *cp = cdp.get();

	D *dp = new D();
	cdp.reset(dp);
	BOOST_CHECK_EQUAL(cdp.get(), dp);
}

BOOST_AUTO_TEST_CASE(testOparrow) {	// operator->
	DeepCopyPtr<C> p(new C(100));
	BOOST_CHECK_EQUAL(p->i_, 100);	// 呼出テスト
}

BOOST_AUTO_TEST_CASE(testOppointer) {	// operator*
	DeepCopyPtr<C> p(new C(111));
	BOOST_CHECK(typeid(*p) == typeid(C &));	// 呼出テスト

	// 素のoperator*()と同じ挙動か?
	C *cp = new D();
	DeepCopyPtr<C> dp = new D();
	BOOST_CHECK(typeid(*dp) == typeid(*cp));
	delete cp;
}

BOOST_AUTO_TEST_CASE(testOpeqeq) {	// operator==
	DeepCopyPtr<C> p1(new C());
	DeepCopyPtr<C> p2(p1);
	BOOST_CHECK_EQUAL(p1==p2, false);
	BOOST_CHECK_EQUAL(p1==p1, true);
}

BOOST_AUTO_TEST_CASE(testOpnoteq) {	// operator!=
	DeepCopyPtr<C> p1(new C());
	DeepCopyPtr<C> p2(p1);
	BOOST_CHECK_EQUAL(p1!=p2, true);
	BOOST_CHECK_EQUAL(p1!=p1, false);
}

BOOST_AUTO_TEST_CASE(testOpequal_sp_toSame) {	// operator=
	// 同じ型同士
	DeepCopyPtr<D> src(new D(1)), dst1, dst2;
	BOOST_REQUIRE(src);
	BOOST_REQUIRE(!dst1);
	BOOST_REQUIRE(!dst2);

	dst1 = dst2 = src;
	BOOST_CHECK(dst1);
	BOOST_CHECK_NE(dst1.get(), src.get());
	BOOST_CHECK_EQUAL(*dst1, *src);
	BOOST_CHECK(dst2);
	BOOST_CHECK_NE(dst2.get(), src.get());
	BOOST_CHECK_EQUAL(*dst2, *src);
	BOOST_CHECK_NE(dst1.get(), dst2.get());
}

BOOST_AUTO_TEST_CASE(testOpequal_sp_toSame_from0) {	// operator=
	// 同じ型のNULLポインタから代入
	DeepCopyPtr<D> lhs(new D(1)), rhs;
	BOOST_REQUIRE(lhs);
	BOOST_REQUIRE(!rhs);
	lhs = rhs;
	BOOST_CHECK(!lhs);
}

BOOST_AUTO_TEST_CASE(testOpequal_sp_toSuper) {	// operator=
	// 継承元の型へ
	DeepCopyPtr<C> c;
	BOOST_REQUIRE(!c);
	DeepCopyPtr<D> d(new D(1));
	BOOST_REQUIRE_EQUAL(d->i_, 1);

	c = d;
	BOOST_CHECK(c);
	BOOST_CHECK_NE(c.get(), d.get());
	BOOST_CHECK_EQUAL(*c, *d);
}

BOOST_AUTO_TEST_CASE(testOpequal_sp_toSuper_from0) {	// operator=
	// 継承先の型のNULLポインタから代入
	DeepCopyPtr<C> lhs(new C(1));
	DeepCopyPtr<D> rhs;
	BOOST_REQUIRE(lhs);
	BOOST_REQUIRE(!rhs);
	lhs = rhs;
	BOOST_CHECK(!lhs);
}

BOOST_AUTO_TEST_CASE(testOpequal_lp_toSame) {	// operator=
	// 同型、素のポインタから
	DeepCopyPtr<D> dsp;
	BOOST_REQUIRE(!dsp);
	D *dlp = new D(1);

	dsp = dlp;
	BOOST_CHECK(dsp);
	BOOST_CHECK_NE(dsp.get(), dlp);
	BOOST_CHECK_EQUAL(dsp->i_, dlp->i_);

	delete dlp;

	dsp = 0;	// NULLポインタ代入
	BOOST_CHECK(!dsp);
}

BOOST_AUTO_TEST_CASE(testOpequal_lp_toSame_from0) {	// operator=
	// 同型の素のNULLポインタから代入
	DeepCopyPtr<D> lhs(new D(1));
	D *rhs = 0;
	BOOST_REQUIRE(lhs);
	lhs = rhs;
	BOOST_CHECK(!lhs);
}

BOOST_AUTO_TEST_CASE(testOpequal_lp_toSuper) {	// operator=
	// 継承元へ、素のポインタから
	DeepCopyPtr<C> csp;
	BOOST_REQUIRE(!csp);
	D *dlp = new D(1);

	csp = dlp;
	BOOST_CHECK(csp);
	BOOST_CHECK_NE(csp.get(), dlp);
	BOOST_CHECK_EQUAL(csp->i_, dlp->i_);

	delete dlp;

	csp = 0;	// NULLポインタ代入
	BOOST_CHECK(!csp);
}

BOOST_AUTO_TEST_CASE(testOpequal_lp_toSuper_from0) {	// operator=
	// 継承先の型の素のNULLポインタから代入
	DeepCopyPtr<C> lhs(new C(1));
	D *rhs = 0;
	BOOST_REQUIRE(lhs);
	lhs = rhs;
	BOOST_CHECK(!rhs);
}

BOOST_AUTO_TEST_CASE(testOpbool) {	// operator bool
	DeepCopyPtr<int> p1, p2(static_cast<int *>(0)), p3(new int());
	BOOST_CHECK_EQUAL(static_cast<bool>(p1), false);
	BOOST_CHECK_EQUAL(static_cast<bool>(p2), false);
	BOOST_CHECK_EQUAL(static_cast<bool>(p3), true);
}

BOOST_AUTO_TEST_SUITE_END()
