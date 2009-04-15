/*
 * copyright (c)2009 http://wamsoft.jp
 * zlib license
 */
#include "sqobjectinfo.h"
#include "sqobject.h"
#include "sqthread.h"
#include <string.h>

namespace sqobject {

// 内容消去
void
ObjectInfo::clear()
{
	HSQUIRRELVM gv = getGlobalVM();
	sq_release(gv,&obj);
	sq_resetobject(&obj);
}

// スタックから取得
void
ObjectInfo::getStack(HSQUIRRELVM v, int idx)
{
	clear();
	HSQUIRRELVM gv = getGlobalVM();
	sq_move(gv, v, idx);
	sq_getstackobj(gv, -1, &obj);
	sq_addref(gv, &obj);
	sq_pop(gv, 1);
}

// スタックから弱参照として取得
void
ObjectInfo::getStackWeak(HSQUIRRELVM v, int idx)
{
	clear();
	HSQUIRRELVM gv = getGlobalVM();
	sq_weakref(v, idx);
	sq_move(gv, v, -1);
	sq_pop(v, 1);
	sq_getstackobj(gv, -1, &obj);
	sq_addref(gv, &obj);
	sq_pop(gv, 1);
}


// コンストラクタ
ObjectInfo::ObjectInfo() {
	sq_resetobject(&obj);
}

// コンストラクタ
ObjectInfo::ObjectInfo(HSQUIRRELVM v, int idx)
{
	sq_resetobject(&obj);
	HSQUIRRELVM gv = getGlobalVM();
	sq_move(gv, v, idx);
	sq_getstackobj(gv, -1, &obj);
	sq_addref(gv, &obj);
	sq_pop(gv, 1);
}

// コピーコンストラクタ
ObjectInfo::ObjectInfo(const ObjectInfo &orig)
{
	HSQUIRRELVM gv = getGlobalVM();
	sq_resetobject(&obj);
	obj = orig.obj;
	sq_addref(gv, &obj);
}

// 代入
ObjectInfo& ObjectInfo::operator =(const ObjectInfo &orig)
{
	HSQUIRRELVM gv = getGlobalVM();
	clear();
	obj = orig.obj;
	sq_addref(gv, &obj);
	return *this;
}

// デストラクタ
ObjectInfo::~ObjectInfo()
{
	clear();
}

// nullか？
bool
ObjectInfo::isNull() const
{
	if (sq_isweakref(obj)) {
		HSQUIRRELVM gv = getGlobalVM();
		sq_pushobject(gv, obj);
		sq_getweakrefval(gv, -1);
		bool ret = sq_gettype(gv, -1) == OT_NULL;
		sq_pop(gv, 2);
		return ret;
	}
	return sq_isnull(obj);
}

// 同じスレッドか？
bool
ObjectInfo::isSameThread(const HSQUIRRELVM v) const
{
	return sq_isthread(obj) && obj._unVal.pThread == v;
}

// スレッドを取得
ObjectInfo::operator HSQUIRRELVM() const
{
	HSQUIRRELVM vm = sq_isthread(obj) ? obj._unVal.pThread : NULL;
	return vm;
}
	
// オブジェクトをPUSH
void
ObjectInfo::push(HSQUIRRELVM v) const
{
    if (sq_isweakref(obj)) {
		sq_pushobject(v, obj);
		sq_getweakrefval(v, -1);
		sq_remove(v, -2);
	} else {
		sq_pushobject(v, obj);
	}
}

// ---------------------------------------------------
// delegate 処理用
// ---------------------------------------------------

// delegate として機能するかどうか
bool
ObjectInfo::isDelegate() const
{
	return (sq_isinstance(obj) || sq_istable(obj));
}

// bindenv させるかどうか
bool
ObjectInfo::isBindDelegate() const
{
	return (sq_isinstance(obj));
}

// ---------------------------------------------------
// データ取得
// ---------------------------------------------------

const SQChar *
ObjectInfo::getString()
{
	if (sq_isstring(obj)) {
		HSQUIRRELVM gv = getGlobalVM();
		const SQChar *mystr;
		sq_pushobject(gv, obj);
		sq_getstring(gv, -1, &mystr);
		sq_pop(gv, 1);
		return mystr;
	}
	return NULL;
}

// ---------------------------------------------------
// wait処理用メソッド
// ---------------------------------------------------

bool
ObjectInfo::isSameString(const SQChar *str) const
{
	if (str && sq_isstring(obj)) {
		HSQUIRRELVM gv = getGlobalVM();
		const SQChar *mystr;
		sq_pushobject(gv, obj);
		sq_getstring(gv, -1, &mystr);
		sq_pop(gv, 1);
		return mystr && scstrcmp(str, mystr) == 0;
	}
	return false;
}

// ---------------------------------------------------
// 配列処理用メソッド
// ---------------------------------------------------


/// 配列として初期化
void
ObjectInfo::initArray(int size)
{
	clear();
	HSQUIRRELVM gv = getGlobalVM();
	sq_newarray(gv, size);
	sq_getstackobj(gv, -1, &obj);
	sq_addref(gv, &obj);
	sq_pop(gv, 1);
}

/// 配列に値を追加
void ObjectInfo::append(HSQUIRRELVM v, int idx)
{
	HSQUIRRELVM gv = getGlobalVM();
	sq_pushobject(gv, obj);
	sq_move(gv, v, idx);
	sq_arrayappend(gv, -2);
	sq_pop(gv,1);
}

/// 配列に値を追加
template<typename T>
void ObjectInfo::append(T value)
{
	HSQUIRRELVM gv = getGlobalVM();
	sq_pushobject(gv, obj);
	pushValue(gv, value);
	sq_arrayappend(gv, -2);
	sq_pop(gv,1);
}

/// 配列に値を挿入
template<typename T>
void ObjectInfo::insert(int index, T value)
{
	HSQUIRRELVM gv = getGlobalVM();
	sq_pushobject(gv, obj);
	pushValue(gv, value);
	sq_arrayinsert(gv, -2, index);
	sq_pop(gv,1);
}

/// 配列に値を格納
template<typename T>
void ObjectInfo::set(int index, T value)
{
	HSQUIRRELVM gv = getGlobalVM();
	sq_pushobject(gv, obj);
	sq_pushinteger(gv, index);
	pushValue(gv, value);
	sq_set(gv, -3);
	sq_pop(gv,1);
}

/// 配列の長さ
int
ObjectInfo::len() const
{
	HSQUIRRELVM gv = getGlobalVM();
	sq_pushobject(gv, obj);
	int ret = sq_getsize(gv,-1);
	sq_pop(gv,1);
	return ret;
}

/**
 * 配列の内容を全部PUSH
 * @param v squirrelVM
 * @return push した数
 */
int
ObjectInfo::pushArray(HSQUIRRELVM v) const
{
	if (!isArray()) {
		return 0;
	}
	HSQUIRRELVM gv = getGlobalVM();
	sq_pushobject(gv, obj);
	int len = sq_getsize(gv,-1);
	for (int i=0;i<len;i++) {
		sq_pushinteger(gv, i);
		sq_get(gv, -2);
		sq_move(v, gv, -1);
		sq_pop(gv, 1);
	}
	sq_pop(gv,1);
	return len;
}

};
