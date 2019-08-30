// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "RiverexpConfig.h"
#include <z3.h>

#ifdef USE_MYMATH
#include "MathFunctions.h"
#endif

#include <iostream>
#include <assert.h>
#include"z3++.h"
using namespace z3;

class Test{};

void example1()
{
    context c;
    expr x = c.int_const("x");
    expr y = c.int_const("y");
    solver s(c);

    s.add(x >= 1);
    s.add(y < x + 3);
    std::cout<<s.check() <<"\n";

    model m = s.get_model();
    std::cout<<m<<"\n";

    // traverse the model
    for (unsigned i = 0; i < m.size(); i++)
    {
      func_decl v = m[i];
      assert(v.arity() == 0);
      std::cout<<v.name() << " = " << m.get_const_interp(v) <<"\n";
    }

    std::cout<<"X + y + 1 = " << m.eval(x + y + 1) << "\n";
}

void example2()
{
    std::cout<<"bit vector example \n";
    context c;
    expr x = c.bv_const("x", 32);
    expr y = c.bv_const("y", 32);
    solver s(c);

    s.add((x ^ y) - 103 == x * y);
    std::cout<<s <<"\n";
    std::cout<<s.check() <<"\n";
    std::cout<<s.get_model() <<"\n";

    Z3_string res = Z3_solver_to_string(c, s);

    context c2;
    solver s2(c2);
    Z3_solver_from_string(c2, s2, res);
    std::cout<<s2<<"\n";
}

void example3()
{
    std::cout<<"two contexts\n";
    context c1, c2;

    expr x = c1.int_const("x");
    expr n = x + 1;
  std::cout<<Z3_translate(c1, n, c2)<<"\n";

    expr n1 = to_expr(c2, Z3_translate(c1, n, c2));
    std::cout<< n1 <<"\n";
}

void opt_example() 
{
    context c;
    optimize opt(c);
    params p(c);
    p.set("priority",c.str_symbol("pareto"));
    opt.set(p);
    expr x = c.int_const("x");
    expr y = c.int_const("y");
    opt.add(10 >= x && x >= 0);
    opt.add(10 >= y && y >= 0);
    opt.add(x + y <= 11);
    optimize::handle h1 = opt.maximize(x);
    optimize::handle h2 = opt.maximize(y);
    while (true) {
        if (sat == opt.check()) {
            std::cout << x << ": " << opt.lower(h1) << " " << y << ": " << opt.lower(h2) << "\n";
        }
        else {
            break;
        }
    }
}

void parse_string() 
{
    std::cout << "parse string\n";
    z3::context c;
    z3::solver s(c);
    //Z3_solver_from_string(c, s, "(declare-const x Int)(assert (> x 10))");
    s.from_string("(declare-const x Int)(assert (> x 10))");
    std::cout << s.check() << "\n";
}

void string_values() 
{
    context c;
    expr s = c.string_val("abc\n\n\0\0", 7);
    std::cout << s << "\n";
    std::string s1 = s.get_string();
    std::cout << s1 << "\n";
}

#include <vector>
#include "BinFormatConcolic.h"

int main (int argc, char *argv[])
{
# if 0  // Z3 experiments
    unsigned major, minor, build, revision;
    Z3_get_version(&major, &minor, &build, &revision);
    printf("Z3 %d.%d.%d.%d\n", major, minor, build, revision);

    example2();

    //example1();
    //example2();
    //example3();
    //opt_example();

    parse_string();
    string_values();

#else  // Parsing experiments


#endif

  return 0;
}

