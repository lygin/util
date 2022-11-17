#include "smart_ptr.h"
#include <iostream>
using std::cout;
using std::endl;

struct MyStruct
{
	MyStruct() = default;
	MyStruct(int a, int b) :a(a), b(b) {}
	int a;
	int b;
};

void func(smart_ptr<MyStruct> xx) {
    //xx(sp) 拷贝构造+1
    //析构xx -1
}
int main()
{
	MyStruct *s = new MyStruct();
	s->a = 10;
	s->b = 20;

	smart_ptr<MyStruct> sp(s);
    func(sp);
    {   
        auto xxx = sp;//这个时候的拷贝赋值是初始化，等价于拷贝构造，即xxx(sp),+1
        xxx = sp;//拷贝赋值 
        //xxx 析构-1
    }

	cout << sp->a << endl;
	cout << sp->b << endl;
	cout << (*sp).a << endl;

	auto sp2 = make_smart<MyStruct>(100, 200);
	cout << sp2->a << endl;
	cout << sp2->b << endl;

	auto p = sp2.release();
	cout << p->a << endl;
	cout << p->b << endl;
	delete p;

	return 0;
}