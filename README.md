# let-anything PPX

This is a PPX (language extension) designed to make _monadic operations_ (think async functions with "await" statements in Javascript if you don't know what a monadic operation is) easy to write and read in ReasonML.

## Installation

- `npm install --save-dev @mrmurphy/let-anything`
- Open up your `bsconfig.json` and add `@mrmurphy/let-anything/ppx` to your `ppx-flags`. It should look something like this:
  ```json
    {
        ...
        "bsc-flags": [...],
        "ppx-flags": ["@mrmurphy/let-anything/ppx", ...],
        "refmt": ...
    }
  ```

## Usage

Simple and sweet, this is a language extension that flattens callbacks. All you need is a module which defines a function called `let_` which takes something to map over, and a callback to do the mapping. For example:

```reasonml
module Option = {
    let let_ = Belt.Option.flatMap;
}
```

Then, when you're working with something you want to map, add a `%<ModuleName>` onto your `let`, and the rest of the lines in the block will be turned into a callback and passed to the mapping function _at compile time_.

For example:

```reasonml
// Assume the `Option` module from above is defined already.

type address = {
    street: option(string)
};

type personalInfo = {
    address: option(address)
};

type user = {
    info: option(personalInfo)
};

// Get the user's street name from a bunch of nested options. If anything is
// None, return None.
let getStreet = (maybeUser: option(user)): option(string) => {
    let%Opt user = maybeUser;
    // Notice that info isn't an option anymore once we use let%Pom!
    let%Opt info = user.info;
    let%Opt address = info.address;
    let%Opt street = info.street;
    Some(street->Js.String.toUpperCase)
};
```

That code is flat, readable, and understandable. Here's an alternative without the syntax sugar:

```reasonml
let getStreet = (maybeUser: option(user)): option(string) => {
    maybeUser->Belt.Option.flatMap(user => {
        user->Belt.Option.flatMap(info => {
            info->Belt.Option.flatMap(address => {
                address->Belt.Option.flatMap(street => {
                    Some(street->Js.String.toUpperCase)
                })
            })
        })
    })
};
```

Much nicer to have the sugar, no? This PPX really shines, though, when we use it to chain async operations, since that has to be done quite a lot in Javascript, especially server-side, and it typically happens multiple times in the middle of large and complex functions.

Here's a more complex example of an async control flow using the [Repromise](https://aantron.github.io/repromise/docs/QuickStart) library to work with Javascript promises:

```reasonml

// Repromise doesn't ship with native support for this PPX, so we simply add our
// own by re-defining the module, including all the stuff from the original
// module, and adding our own function.
module Repromise = {
    include Repromise;
    let let_ = Repromise.andThen;

    // This is totally optional. It can be nice sometimes to return a
    // non-promise value at the end of a function and have it automatically
    // wrapped.
    module Wrap = {
        let let_ = Repromise.map;
    }
}

let logUserIn = (email: string, password: string) => {
    // Assume this is a function that returns a promise of a hash.
    let%Repromise hash = UserService.hashPassword(password)
    let%Repromise maybeUser = UserService.findUserForEmailAndHash(email, hash);
    let result = switch (maybeUser) {
    | Some(user) =>
        // It even works inside of a switch expression!
        // Here you can see we're using ".Wrap" to automatically wrap our result
        // in a promise.
        let%Repromise.Wrap apiToken = TokenService.generateForUser(user.id);
        Ok( user.firstName, apiToken )
    | None =>
        // We resolve a promise here to match the branch above.
        Error("Sorry, no user found for that email & password combination")->Repromise.resolved
    };

    // Since let_ is defined as "andThen" we've got to remember to return a promise
    // at the end of the function! Remember, all the lines after each let% just get
    // turned into a callback!
    Repromise.resolved(result)
};
```

There's a whole lot that can be done with this PPX. You can even go a little crazy and write a module like `AsyncOption` that would specifically handle optional values inside of promises. I've done that in my main project at work, but in practice I use it seldom, so don't go too crazy. Keeping it simple will get you a long, long way.

## Notes

**A Note about Native VS Bucklescript**
This is specifically designed to be helpful with writing Javascript code through ReasonML and Bucklescript. Native OCaml 4.08 implemented a native monadic sugar syntax. So if you're writing native code, I'd suggest skipping this PPX and waiting until [this PR](https://github.com/facebook/reason/pull/2487) lands in Reason, and then adopting the new syntax.

**A Note about Windows**
I don't develop on Windows and neither does my company, so I've only taken the time to precomile binaries for linux and osx in this package. If you're a Windows user and would like to use this PPX, I'd love a pull-request that moves the project from Travis to Azure Pipelines and builds for all three platforms.

## Credit

This PPX was created by @jaredly and upgraded to the latest OCaml by @anmonteiro. I have updated Jared's work with Antonio's changes, re-packaged it to build with `esy` and precompiled binaries for osx and linux. I've also written this readme to describe the most common use-case for this PPX. More features are available but undocumented in this readme. You can see them here: https://github.com/jaredly/let-anything. Unfortunately I didn't write the PPX, and I'm still pretty baffled by OCaml's AST and the tools surrounding it, so I may not be very helpful in closing issues 😅.
