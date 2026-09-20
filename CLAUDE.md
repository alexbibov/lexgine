# Build conventions
* Build directories live outside the source tree at `../cmake-builds/lexgine/<preset-name>`, e.g.
  `C:/Repositories/cmake-builds/lexgine/vs` for the `vs` preset. Never configure into `build/` inside the repo.
* Build with `cmake --build ../cmake-builds/lexgine/<preset-name> --config <Debug|Release>`.
* Demo and test binaries land in `<build-dir>/bin/<config>/`; the demo is `swe.exe`.

# Commit conventions
* Commit messages are short and to the point.
* Do _NOT_ add any AI attribution to commit messages or pull request descriptions. No `Co-Authored-By: Claude`,
  no "Generated with Claude Code", no equivalent trailer or footer in any wording.

# Project conventions
* We use spaces for indentation. One scope indentation is 4 consequent spaces.
* Class names use pattern `ThisIsMyClass`
* Function names use pattern `myFunction(...)`
* Class members must use prefix `m_` for instance-level fields, `s_` for static fields and `c_` for constant fields
* Variable names use `snail_case`
* Helper functions within translation units must be put to anonymous namespaces. The closing scope of the anonymous namespace must be followed by `// namespace` comment, i.e.
```C++
namespace
{
    void myFunction1()
    {
        ...
    }

    void myFunction2()
    {
        ...
    }
}  // namespace
```
* The code is the ultimate and final documentation. Do _NOT_ add obvious comments to implementations. The only comments worth adding to implementation is links to external research papers if the implementation follows non-trivial algorithm, which is difficult to figure out from the code itself. 
* Comments next to function and class definitions are allowed but _MUST_ act as documentation, so it is _REQUIRED_ that their format follows Doxygen documenting standards. Being documentation they describe what certain API does and do not describe how it does that
* Class definitions _MUST_ follow conventions described below:
    * First declarations are `private` declarations that _MUST_ be declared before the `public` declarations due to name look up
    * Then follow the `public` declarations followed by `protected` and another section of `private` declarations, which do not need to appear before the public APIs
    * Within each access section declarations appear in the following order: first are listed `static` and `constexpr` variables, then `const` variables, then nested types followed by function declarations and instance-level fields. Each of these sections _MUST_ be preceded by its corresponding access level (e.g. within, say, `private` section the keyword `private` _MUST_ appear before the set of `static` fields then before the set of private functions and so on).
    * The very first `private` keyword within a `class` (not a `struct`!) declaration _MAY_ be omitted
    * Below is given an exampled of acceptable class declaration

```C++
class MyClass
{
    static int s_my_field1;
    constexpr char s_my_field2 = 'c';

private:
    using SetOfInts = std::unordered_set<int>;

public:
    SetOfInts const& getMySetOfInts() const;

private:
    struct MyNestedStruct
    {
        ...
    };

private:
    void foo();
    void foo1();

private:
    SetOfInts m_set_of_ints;
};
```