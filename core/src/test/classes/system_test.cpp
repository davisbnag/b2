//This file is part of Bertini 2.0.
//
//system_test.cpp is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//system_test.cpp is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with system_test.cpp.  If not, see <http://www.gnu.org/licenses/>.
//

//  system_test.cpp
//
//  Created by Collins, James B. on 4/30/15.
//  Copyright (c) 2015 West Texas A&M University. All rights reserved.
//
// also modified by
//  Daniel Brake
//  University of Notre Dame
//  ACMS
//  Spring, Summer 2015



#include <boost/test/unit_test.hpp>



#include "system.hpp"
#include "system_parsing.hpp"

using System = bertini::System;
using Var = std::shared_ptr<bertini::Variable>;
using VariableGroup = bertini::VariableGroup;


template<typename T> using Vec = Eigen::Matrix<T, Eigen::Dynamic, 1>;
template<typename T> using Mat = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;


BOOST_AUTO_TEST_SUITE(system_class)


BOOST_AUTO_TEST_CASE(system_make_a_system_at_all)
{
	System S;
}

BOOST_AUTO_TEST_CASE(system_create_parser)
{
	System sys;
	std::string str = "variable_group x, y, z; \nfunction f1, f2;\n  f1 = x*y*z;\n f2 = x+y+z;\n";

	std::string::const_iterator iter = str.begin();
	std::string::const_iterator end = str.end();

	bertini::SystemParser<std::string::const_iterator> S;

	bool s = phrase_parse(iter, end, S,boost::spirit::ascii::space, sys);

	BOOST_CHECK(s && iter==end);
	BOOST_CHECK(sys.IsHomogeneous());

}






BOOST_AUTO_TEST_CASE(system_parse_xyz_f1f2_t_pq)
{

	std::string str = "variable_group x, y, z;\n function f1, f2;\n pathvariable t;\n parameter p, q;\n p = t;\n q = 1-t;\n f1 = x*y*z;\n\nf2 = p*q*x - 2^(-5);\n";




	bertini::System sys;
	std::string::const_iterator iter = str.begin();
	std::string::const_iterator end = str.end();
	bertini::SystemParser<std::string::const_iterator> S;
	bool s = phrase_parse(iter, end, S, boost::spirit::ascii::space, sys);
	BOOST_CHECK(s && iter==end);

}



BOOST_AUTO_TEST_CASE(system_parse_with_subfunctions)
{

	std::string str = "function f; variable_group x1, x2; y = x1*x2; f = y*y;";

	bertini::System sys;
	std::string::const_iterator iter = str.begin();
	std::string::const_iterator end = str.end();
	bertini::SystemParser<std::string::const_iterator> S;
	bool s = phrase_parse(iter, end, S, boost::spirit::ascii::space, sys);
	BOOST_CHECK(s && iter==end);

	BOOST_CHECK(sys.IsHomogeneous());
}


BOOST_AUTO_TEST_CASE(system_parse_around_the_unit_circle)
{
	std::string str =
 "variable z;\nfunction H;\nparameter q1,q2;\npathvariable t;\nq1 = cos(2*Pi*(1-t));\nq2 = sin(2*Pi*(1-t));\ns = q1 + I*q2;\nH = z^2 - s;\n";



	bertini::System sys;
	std::string::const_iterator iter = str.begin();
	std::string::const_iterator end = str.end();
	bertini::SystemParser<std::string::const_iterator> S;
	bool s = phrase_parse(iter, end, S, boost::spirit::ascii::space, sys);
	BOOST_CHECK(s && iter==end);

	BOOST_CHECK(!sys.IsHomogeneous());
}




BOOST_AUTO_TEST_CASE(system_parse_around_the_unit_circle_alt)
{
	std::string str = " variable z; function H; parameter s; pathvariable t; s = exp(2*Pi*I*(1-t)); H = z^2 - s; ";


	bertini::System sys;
	std::string::const_iterator iter = str.begin();
	std::string::const_iterator end = str.end();
	bertini::SystemParser<std::string::const_iterator> S;
	bool s = phrase_parse(iter, end, S, boost::spirit::ascii::space, sys);
	BOOST_CHECK(s && iter==end);

}



//TODO: uncomment this test once error handling has been done for the parsers.
// BOOST_AUTO_TEST_CASE(system_parse_x_y_not_xy)
// {
// 	std::string str = " variable x, y; function f; f = xy;";


// 	bertini::System sys;
// 	std::string::const_iterator iter = str.begin();
// 	std::string::const_iterator end = str.end();
// 	bertini::SystemParser<std::string::const_iterator> S;
// 	bool s = phrase_parse(iter, end, S, boost::spirit::ascii::space, sys);
// 	BOOST_CHECK(!s && iter!=end);

// }




BOOST_AUTO_TEST_CASE(system_differentiate_x)
{
	std::shared_ptr<bertini::Variable> x = std::make_shared<bertini::Variable>("x");
	auto f1 = pow(x,2);
	auto f2 = x-1;

	bertini::System S;
	S.AddUngroupedVariable(x);
	S.AddFunction(f1);
	S.AddFunction(f2);

	Vec<dbl> v(1);
	v << 1.0;

	auto J = S.Jacobian(v);

	BOOST_CHECK_EQUAL(J(0),2.0);
	BOOST_CHECK_EQUAL(J(1),1.0);
}



BOOST_AUTO_TEST_CASE(system_differentiate_x_and_y)
{
	std::shared_ptr<bertini::Variable> x = std::make_shared<bertini::Variable>("x");
	std::shared_ptr<bertini::Variable> y = std::make_shared<bertini::Variable>("y");
	auto f1 = pow(x,2)*y/2;
	auto f2 = x-y;

	bertini::System S;
	S.AddUngroupedVariable(x);
	S.AddUngroupedVariable(y);
	S.AddFunction(f1);
	S.AddFunction(f2);

	Vec<dbl> v(2);
	v << 1.0 , 2.0;

	auto J = S.Jacobian(v);

	BOOST_CHECK_THROW(S.Jacobian(v,dbl(0.5)), std::runtime_error);
}



BOOST_AUTO_TEST_CASE(system_differentiate_x_and_t)
{
	std::shared_ptr<bertini::Variable> x = std::make_shared<bertini::Variable>("x");
	std::shared_ptr<bertini::Variable> t = std::make_shared<bertini::Variable>("t");
	auto f1 = (1-t)*x + t*(1-x);
	auto f2 = x-t;

	bertini::System S;
	S.AddUngroupedVariable(x);
	S.AddPathVariable(t);
	S.AddFunction(f1);
	S.AddFunction(f2);

	Vec<dbl> v(1);
	v << 1.0;
	dbl time(0.5,0.2);
	auto J = S.Jacobian(v,time);

	BOOST_CHECK_THROW(S.Jacobian(v), std::runtime_error);
}



BOOST_AUTO_TEST_CASE(system_homogenize_multiple_variable_groups)
{
	Var x1 = std::make_shared<bertini::Variable>("x1");
	Var x2 = std::make_shared<bertini::Variable>("x2");

	Var y1 = std::make_shared<bertini::Variable>("y1");
	Var y2 = std::make_shared<bertini::Variable>("y2");


	bertini::VariableGroup v1{x1, x2};
	bertini::VariableGroup v2{y1, y2};

	auto f1 = x1*y1 + x1;
	auto f2 = x2*x1 + y1*y2 + x1 + y2 - 1;


	bertini::System S;
	S.AddVariableGroup(v1);
	S.AddVariableGroup(v2);
	
	S.AddFunction(f1);
	S.AddFunction(f2);

	BOOST_CHECK(!S.IsHomogeneous());

	BOOST_CHECK_EQUAL(S.NumHomVariables(),0);

	S.Homogenize();

	BOOST_CHECK(S.IsHomogeneous());
}



BOOST_AUTO_TEST_CASE(system_reorder_by_degree_decreasing)
{
	Var x1 = std::make_shared<bertini::Variable>("x1");
	Var x2 = std::make_shared<bertini::Variable>("x2");

	Var y1 = std::make_shared<bertini::Variable>("y1");
	Var y2 = std::make_shared<bertini::Variable>("y2");


	bertini::VariableGroup v1{x1, x2};
	bertini::VariableGroup v2{y1, y2};

	auto f1 = x1*y1 + x1;
	auto f2 = x2*pow(x1,2) + y1*y2 + x1 + y2 - 1;

	bertini::System S;
	S.AddVariableGroup(v1);
	S.AddVariableGroup(v2);
	
	S.AddFunction(f1); // deg 2
	S.AddFunction(f2); // deg 3 
	S.AddFunction(pow(x1,4) + pow(y2,5)); // deg 5
	S.AddFunction(x1 + x2 + y1 + y2); // deg 1


	BOOST_CHECK(!S.IsHomogeneous());
	S.ReorderFunctionsByDegreeDecreasing();

	auto degs = S.Degrees();

	for (auto d = degs.begin(); d != degs.end()-1; d++)
	{
		BOOST_CHECK(*d >= *(d+1));
	}

}




BOOST_AUTO_TEST_CASE(system_reorder_by_degree_increasing)
{
	Var x1 = std::make_shared<bertini::Variable>("x1");
	Var x2 = std::make_shared<bertini::Variable>("x2");

	Var y1 = std::make_shared<bertini::Variable>("y1");
	Var y2 = std::make_shared<bertini::Variable>("y2");


	bertini::VariableGroup v1{x1, x2};
	bertini::VariableGroup v2{y1, y2};

	auto f1 = x1*y1 + x1;
	auto f2 = x2*pow(x1,2) + y1*y2 + x1 + y2 - 1;


	bertini::System S;
	S.AddVariableGroup(v1);
	S.AddVariableGroup(v2);
	
	S.AddFunction(f1);
	S.AddFunction(f2);

	BOOST_CHECK(!S.IsHomogeneous());
	S.ReorderFunctionsByDegreeIncreasing();

	auto degs = S.Degrees();

	for (auto d = degs.begin(); d != degs.end()-1; d++)
	{
		BOOST_CHECK(*d <= *(d+1));
	}

}






BOOST_AUTO_TEST_CASE(system_evaluate_double)
{

	std::string str = "function f; variable_group x1, x2; y = x1*x2; f = y*y;";

	bertini::System sys;
	std::string::const_iterator iter = str.begin();
	std::string::const_iterator end = str.end();
	bertini::SystemParser<std::string::const_iterator> S;
	phrase_parse(iter, end, S, boost::spirit::ascii::space, sys);

	Vec<dbl> values(2);

	values(0) = dbl(2.0);
	values(1) = dbl(3.0);

	Vec<dbl> v = sys.Eval(values);

	BOOST_CHECK_EQUAL(v(0), 36.0);


	auto J = sys.Jacobian(values);

	double x1 = 2;
	double x2 = 3;

	BOOST_CHECK_EQUAL(J(0,0), 2*x1*x2*x2);
	BOOST_CHECK_EQUAL(J(0,1), x1*x1*2*x2);
}



BOOST_AUTO_TEST_CASE(system_evaluate_mpfr)
{

	std::string str = "function f; variable_group x1, x2; y = x1*x2; f = y*y;";

	bertini::System sys;
	std::string::const_iterator iter = str.begin();
	std::string::const_iterator end = str.end();
	bertini::SystemParser<std::string::const_iterator> S;
	phrase_parse(iter, end, S, boost::spirit::ascii::space, sys);

	Vec<mpfr> values(2);

	values(0) = mpfr(2.0);
	values(1) = mpfr(3.0);

	Vec<mpfr> v = sys.Eval(values);

	BOOST_CHECK_EQUAL(v(0), mpfr(36.0));


	auto J = sys.Jacobian(values);

	mpfr x1 = 2;
	mpfr x2 = 3;
	
	BOOST_CHECK_EQUAL(J(0,0), mpfr(2.0)*x1*x2*x2);
	BOOST_CHECK_EQUAL(J(0,1), x1*x1*mpfr(2.0)*x2);
}





BOOST_AUTO_TEST_CASE(add_two_systems)
{
	bertini::System sys1, sys2;
	Var x = std::make_shared<bertini::Variable>("x"), y = std::make_shared<bertini::Variable>("y");

	VariableGroup vars;
	vars.push_back(x); vars.push_back(y);

	sys1.AddVariableGroup(vars);  
	sys1.AddFunction(y+1);
	sys1.AddFunction(x*y);

	sys2.AddVariableGroup(vars);  
	sys2.AddFunction(-y-1);
	sys2.AddFunction(-x*y);

	sys1+=sys2;


	Vec<dbl> values(2);

	values << dbl(2.0), dbl(3.0);

	auto v = sys1.Eval(values);

	BOOST_CHECK_EQUAL(v(0), 0.0);
	BOOST_CHECK_EQUAL(v(1), 0.0);

	auto deg = sys1.Degrees();

	BOOST_CHECK_EQUAL(deg.size(),2);
	if (deg.size()==2)
	{
		BOOST_CHECK_EQUAL(deg[0],1);
		BOOST_CHECK_EQUAL(deg[1],2);
	}


}
BOOST_AUTO_TEST_SUITE_END()



