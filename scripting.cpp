/*  Copyright (c) 2013, Abdullah A. Hassan <voodooattack@hotmail.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 *  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "scripting.hpp"
#include <v8.h>

namespace ADWIF
{
  void Scripting::createContext(v8::Persistent<v8::Context> & out, v8::Isolate * isolate)
  {
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Context> context = v8::Context::New(isolate);

    out.Reset(isolate, context);
  }

  bool Scripting::executeScript(const std::string & source, std::string & exception, v8::Handle<v8::Value> & result /*, v8::Isolate * isolate */)
  {
    //v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Script> script = v8::Script::Compile(v8::String::New(source.c_str(), source.size()));

    v8::TryCatch try_catch;

    if (script.IsEmpty())
    {
      v8::String::AsciiValue error(try_catch.Exception());
      exception = std::string(*error, error.length());
      return false;
    }

    result = script->Run();

    if (result.IsEmpty())
    {
      v8::String::AsciiValue error(try_catch.Exception());
      exception = std::string(*error, error.length());
      return false;
    }

    return true;
  }

  TemplateBuilder::TemplateBuilder(v8::Isolate * isolate) : myIsolate(isolate)
  {
    v8::HandleScope handle_scope(myIsolate);
    v8::Handle<v8::ObjectTemplate> templ = v8::ObjectTemplate::New();
    Scripting::HandleToPersistent(templ, myTemplate, myIsolate);
  }

  TemplateBuilder & TemplateBuilder::accessor(
    const std::string & name,
    v8::AccessorGetterCallback getter,
    v8::AccessorSetterCallback setter,
    v8::Handle< v8::Value > data,
    v8::AccessControl settings,
    v8::PropertyAttribute attribute,
    v8::Handle<v8::AccessorSignature> signature)
  {
    v8::HandleScope handle_scope(myIsolate);
    v8::Handle<v8::ObjectTemplate> templ = Scripting::PersistentToHandle(myTemplate, myIsolate);
    templ->SetAccessor(v8::String::New(name.c_str(), name.size()), getter, setter, data, settings, attribute, signature);
    return *this;
  }

  TemplateBuilder & TemplateBuilder::indexedPropertyHandler(
    v8::IndexedPropertyGetterCallback getter,
    v8::IndexedPropertySetterCallback setter,
    v8::IndexedPropertyQueryCallback query,
    v8::IndexedPropertyDeleterCallback deleter,
    v8::IndexedPropertyEnumeratorCallback enumerator,
    v8::Handle< v8::Value > data)
  {
    v8::HandleScope handle_scope(myIsolate);
    v8::Handle<v8::ObjectTemplate> templ = Scripting::PersistentToHandle(myTemplate, myIsolate);
    templ->SetIndexedPropertyHandler(getter, setter, query, deleter, enumerator, data);
    return *this;
  }

  v8::Handle< v8::ObjectTemplate > TemplateBuilder::getTemplate()
  {
    v8::HandleScope handle_scope(myIsolate);
    v8::Handle<v8::ObjectTemplate> templ = Scripting::PersistentToHandle(myTemplate);
    return handle_scope.Close(templ);
  }

  void TemplateBuilder::getTemplate(v8::Persistent< v8::ObjectTemplate > & out)
  {
    v8::HandleScope handle_scope(myIsolate);
    out.Reset(myIsolate, myTemplate);
  }

}

