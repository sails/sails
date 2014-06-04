#include <stdio.h>
#include <gtest/gtest.h>
#include <memory>
#include <list>

class Test_Ptr{
public:
    ~Test_Ptr() {
	printf("delete !\n");
    }
    int x;
};

TEST(shared_pointer, use_count)
{
    {
	std::shared_ptr<Test_Ptr> p1(new Test_Ptr());
	printf("p1 use count:%ld\n", p1.use_count());
    }

    {
	printf("list clear\n");
	std::list<Test_Ptr> l;
	Test_Ptr test;
	l.push_back(test);
	l.clear();
    }
    
    {
	printf("list clear shared_ptr\n");
	std::list<std::shared_ptr<Test_Ptr>> l;
	{
	    std::shared_ptr<Test_Ptr> test(new Test_Ptr());
	    l.push_back(test);
	}
	l.clear();
    }
    printf("end\n");
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}










