/*
   MIT License

   Copyright (c) 2021 Jordan Vrtanoski

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   */

#include <memory>
#include <iostream>
#include <vector>

#include "LuaLibrary.hpp"

using namespace LuaCpp::Registry;
using namespace LuaCpp::Engine;

std::string LuaLibrary::getName() {
	return name;
}

std::string LuaLibrary::getMetaTableName()
{
	return metaTableName;
}

void LuaLibrary::setName(const std::string &_name) {
	name = std::move(_name);
}

void LuaLibrary::AddCMetaMethod(const std::string &name, lua_CFunction cfunction) {
	AddCMetaMethod(name, cfunction, false);
}

void LuaLibrary::AddCMetaMethod(const std::string &name, lua_CFunction cfunction, bool replace)
{
	// if we want to replace already existing element, we have to remove old one from list first:
	if(replace)
	{
		metaMethods.erase(name);
	}

	if(!Exists_m(name)) 
	{
		std::unique_ptr<LuaCFunction> func = std::make_unique<LuaCFunction>(cfunction);
		func->setName(name);
		metaMethods.insert(std::make_pair(name, std::move(*func)));
	}
}

void LuaLibrary::AddCMethod(const std::string &name, lua_CFunction cfunction) {
	AddCMethod(name, cfunction, false);
}

void LuaLibrary::AddCMethod(const std::string &name, lua_CFunction cfunction, bool replace) 
{
	// if we want to replace already existing element, we have to remove old one from list first:
	if(replace)
	{
		methods.erase(name);
	}

	if(!Exists_m(name))
	{
		std::unique_ptr<LuaCFunction> func = std::make_unique<LuaCFunction>(cfunction);
		func->setName(name);
		methods.insert(std::make_pair(name, std::move(*func)));
	}
}

void LuaLibrary::AddCFunction(const std::string &name, lua_CFunction cfunction) {
	AddCFunction(name, cfunction, false);	
}

void LuaLibrary::AddCFunction(const std::string &name, lua_CFunction cfunction, bool replace) 
{
	// if we want to replace already existing element, we have to remove old one from list first:
	if(replace)
	{
		functions.erase(name);
	}

	if(!Exists_f(name)) 
	{
		std::unique_ptr<LuaCFunction> func = std::make_unique<LuaCFunction>(cfunction);
		func->setName(name);
		functions.insert(std::make_pair(name, std::move(*func)));
	}
}

lua_CFunction LuaLibrary::getLibMethod(const std::string &name)
{
	return methods.at(name).getCFunction();
}


lua_CFunction LuaLibrary::getLibFunction(const std::string &name)
{
	return functions.at(name).getCFunction();
}

int LuaLibrary::RegisterFunctions(LuaState &L) 
{
	// delete potentially already existing library:
	lua_pushnil(L);
	lua_setglobal(L, name.c_str());

	// Create the metatable and put it on the stack:
    luaL_newmetatable(L, metaTableName.c_str());

	// create table of all meta-methods:
	std::vector<luaL_Reg> arrayMetaMeth(metaMethods.size() + 1);
	int count = 0;

	for (auto & x : metaMethods) {
		LuaCFunction& lcf = x.second;
		arrayMetaMeth[count].name = x.first.c_str();
		arrayMetaMeth[count].func = lcf.getCFunction();
		count++;
	}
	
	arrayMetaMeth[count].name = NULL;
	arrayMetaMeth[count].func = NULL;

	// create table of all functions:
	std::vector<luaL_Reg> arraylib_f(functions.size()+1);
	count = 0;

	for (auto & x : functions) {
		LuaCFunction& lcf = x.second;
		arraylib_f[count].name = x.first.c_str();
		arraylib_f[count].func = lcf.getCFunction();
		count++;
	}
	
	arraylib_f[count].name = NULL;
	arraylib_f[count].func = NULL;

	// create table of all methods:
	std::vector<luaL_Reg> arraylib_m(methods.size()+1);
	count = 0;

	for (auto & x : methods) {
		LuaCFunction& lcf = x.second;
		arraylib_m[count].name = x.first.c_str();
		arraylib_m[count].func = lcf.getCFunction();
		count++;
	}
	
	arraylib_m[count].name = NULL;
	arraylib_m[count].func = NULL;

	// add metamethods to new metatable:
	luaL_setfuncs(L, arrayMetaMeth.data(), 0);

	// create method table:
	luaL_newlibtable(L, arraylib_m);

	// Set the methods to the metatable that should be accessed via object:func:
    luaL_setfuncs(L, arraylib_m.data(), 0);

	// Pop the first metatable off the stack and assign it to __index of the second one:
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);  /* pop metatable */

	// Register the object.func functions into the table that is at the top of the stack:
	luaL_newlib(L, arraylib_f.data());
	lua_setglobal(L,name.c_str());

	return 0;
}
