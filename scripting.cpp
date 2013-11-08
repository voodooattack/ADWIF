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

