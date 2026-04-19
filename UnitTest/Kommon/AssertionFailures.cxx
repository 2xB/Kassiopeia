#include "UnitTest.h"

#include <stdexcept>

namespace
{
class MockVector
{
  public:
    explicit MockVector(double value) : fValue(value) {}

    MockVector operator-(const MockVector& other) const
    {
        return MockVector(fValue - other.fValue);
    }

    double Magnitude() const
    {
        return std::fabs(fValue);
    }

  private:
    double fValue;
};
}  // namespace

TEST(UnitTestAssertionFailureSuite, ExpectEq)
{
    EXPECT_EQ(1, 2);
}

TEST(UnitTestAssertionFailureSuite, AssertEq)
{
    ASSERT_EQ(1, 2);
}

TEST(UnitTestAssertionFailureSuite, ExpectNe)
{
    EXPECT_NE(1, 1);
}

TEST(UnitTestAssertionFailureSuite, AssertNe)
{
    ASSERT_NE(1, 1);
}

TEST(UnitTestAssertionFailureSuite, ExpectLt)
{
    EXPECT_LT(2, 1);
}

TEST(UnitTestAssertionFailureSuite, AssertLt)
{
    ASSERT_LT(2, 1);
}

TEST(UnitTestAssertionFailureSuite, ExpectLe)
{
    EXPECT_LE(2, 1);
}

TEST(UnitTestAssertionFailureSuite, AssertLe)
{
    ASSERT_LE(2, 1);
}

TEST(UnitTestAssertionFailureSuite, ExpectGt)
{
    EXPECT_GT(1, 2);
}

TEST(UnitTestAssertionFailureSuite, AssertGt)
{
    ASSERT_GT(1, 2);
}

TEST(UnitTestAssertionFailureSuite, ExpectGe)
{
    EXPECT_GE(1, 2);
}

TEST(UnitTestAssertionFailureSuite, AssertGe)
{
    ASSERT_GE(1, 2);
}

TEST(UnitTestAssertionFailureSuite, ExpectTrue)
{
    EXPECT_TRUE(false);
}

TEST(UnitTestAssertionFailureSuite, AssertTrue)
{
    ASSERT_TRUE(false);
}

TEST(UnitTestAssertionFailureSuite, ExpectFalse)
{
    EXPECT_FALSE(true);
}

TEST(UnitTestAssertionFailureSuite, AssertFalse)
{
    ASSERT_FALSE(true);
}

TEST(UnitTestAssertionFailureSuite, ExpectNear)
{
    EXPECT_NEAR(1.0, 2.0, 1e-6);
}

TEST(UnitTestAssertionFailureSuite, AssertNear)
{
    ASSERT_NEAR(1.0, 2.0, 1e-6);
}

TEST(UnitTestAssertionFailureSuite, ExpectDoubleEq)
{
    EXPECT_DOUBLE_EQ(1.0, 2.0);
}

TEST(UnitTestAssertionFailureSuite, AssertDoubleEq)
{
    ASSERT_DOUBLE_EQ(1.0, 2.0);
}

TEST(UnitTestAssertionFailureSuite, ExpectDoubleLt)
{
    EXPECT_DOUBLE_LT(2.0, 1.0);
}

TEST(UnitTestAssertionFailureSuite, AssertDoubleLt)
{
    ASSERT_DOUBLE_LT(2.0, 1.0);
}

TEST(UnitTestAssertionFailureSuite, ExpectThrow)
{
    EXPECT_THROW((void) 0, std::runtime_error);
}

TEST(UnitTestAssertionFailureSuite, AssertThrow)
{
    ASSERT_THROW((void) 0, std::runtime_error);
}

TEST(UnitTestAssertionFailureSuite, AssertAnyThrow)
{
    ASSERT_ANY_THROW((void) 0);
}

TEST(UnitTestAssertionFailureSuite, ExpectPtr)
{
    int* ptr = nullptr;
    EXPECT_PTR(ptr);
}

TEST(UnitTestAssertionFailureSuite, AssertPtr)
{
    int* ptr = nullptr;
    ASSERT_PTR(ptr);
}

TEST(UnitTestAssertionFailureSuite, ExpectNull)
{
    int value = 0;
    int* ptr = &value;
    EXPECT_NULL(ptr);
}

TEST(UnitTestAssertionFailureSuite, AssertNull)
{
    int value = 0;
    int* ptr = &value;
    ASSERT_NULL(ptr);
}

TEST(UnitTestAssertionFailureSuite, ExpectStringEq)
{
    EXPECT_STRING_EQ("a", "b");
}

TEST(UnitTestAssertionFailureSuite, AssertStringEq)
{
    ASSERT_STRING_EQ("a", "b");
}

TEST(UnitTestAssertionFailureSuite, ExpectVectorNull)
{
    EXPECT_VECTOR_NULL(MockVector(1.0));
}

TEST(UnitTestAssertionFailureSuite, AssertVectorNull)
{
    ASSERT_VECTOR_NULL(MockVector(1.0));
}

TEST(UnitTestAssertionFailureSuite, ExpectVectorNear)
{
    EXPECT_VECTOR_NEAR(MockVector(1.0), MockVector(2.0));
}

TEST(UnitTestAssertionFailureSuite, AssertVectorNear)
{
    ASSERT_VECTOR_NEAR(MockVector(1.0), MockVector(2.0));
}
