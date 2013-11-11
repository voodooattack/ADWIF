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

#ifndef SCRIPTING_HPP
#define SCRIPTING_HPP

#include <v8.h>
#include <string>

namespace ADWIF
{
  class Scripting
  {
  public:
    static void createContext(v8::Persistent<v8::Context> & out, v8::Isolate * isolate = v8::Isolate::GetCurrent());
    static bool executeScript(const std::string & source, std::string & exception, v8::Handle<v8::Value> & result /*, v8::Isolate * isolate = v8::Isolate::GetCurrent() */);

    template<class T>
    static v8::Handle<T> PersistentToHandle(v8::Persistent<T> & persistent, v8::Isolate * isolate = v8::Isolate::GetCurrent())
    {
      v8::HandleScope handle_scope(isolate);
      v8::Handle<T> templ = v8::Local<T>::New(isolate, persistent);
      return handle_scope.Close(templ);
    }

    template<class T>
    static void HandleToPersistent(v8::Handle<T> handle, v8::Persistent<T> & persistent, v8::Isolate * isolate = v8::Isolate::GetCurrent())
    {
      v8::HandleScope handle_scope(isolate);
      persistent.Reset(isolate, handle);
    }

    template<class T>
    static v8::Handle<v8::Object> wrap(T * object, v8::Handle<v8::ObjectTemplate> templ, v8::Isolate * isolate = v8::Isolate::GetCurrent())
    {
      v8::HandleScope handle_scope(isolate);
      v8::Handle<v8::Object> result = templ->NewInstance();
      v8::Handle<v8::External> wrapped = v8::External::New(object);
      result->SetInternalField(0, wrapped);
      return handle_scope.Close(result);
    }

    template<class T>
    static v8::Handle<v8::Object> wrap(T * object, v8::Persistent<v8::ObjectTemplate> templ, v8::Isolate * isolate = v8::Isolate::GetCurrent())
    {
      v8::HandleScope handle_scope(isolate);
      v8::Handle<T> temp = PersistentToHandle(templ);
      v8::Handle<v8::Object> result = temp->NewInstance();
      v8::Handle<v8::External> wrapped = v8::External::New(object);
      result->SetInternalField(0, wrapped);
      return handle_scope.Close(result);
    }

    template<class T>
    static T * unwrap(v8::Handle<v8::Object> object)
    {
      v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(object->GetInternalField(0));
      void * ptr = field->Value();
      return reinterpret_cast<T *>(ptr);
    }

  };

  class TemplateBuilder
  {
  public:
    TemplateBuilder(v8::Isolate * isolate = v8::Isolate::GetCurrent());

    TemplateBuilder & accessor(
      const std::string & name,
      v8::AccessorGetterCallback getter,
      v8::AccessorSetterCallback setter = nullptr,
      v8::Handle<v8::Value> data = v8::Handle<v8::Value>(),
      v8::AccessControl settings = v8::DEFAULT,
      v8::PropertyAttribute attribute = v8::None,
      v8::Handle<v8::AccessorSignature> signature = v8::Handle<v8::AccessorSignature>());

    TemplateBuilder & indexedPropertyHandler(
      v8::IndexedPropertyGetterCallback getter,
      v8::IndexedPropertySetterCallback setter = nullptr,
      v8::IndexedPropertyQueryCallback query = nullptr,
      v8::IndexedPropertyDeleterCallback deleter = nullptr,
      v8::IndexedPropertyEnumeratorCallback enumerator = nullptr,
      v8::Handle<v8::Value> data = v8::Handle<v8::Value>());

    v8::Handle<v8::ObjectTemplate> getTemplate();
    void getTemplate(v8::Persistent<v8::ObjectTemplate> & out);

  private:
    v8::Persistent<v8::ObjectTemplate> myTemplate;
    v8::Isolate * myIsolate;
  };
}

#endif // SCRIPTING_HPP
