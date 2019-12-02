# bs-let

![A woman knitting some code](https://github.com/reasonml-labs/bs-let/raw/master/artwork/eastwood-page-under-construction.png)

This is a PPX (language extension) designed to make _monadic operations_ (think async functions with "await" statements in Javascript if you don't know what a monadic operation is) easy to write and read in ReasonML.

## Warning: Experimental Project

This is package is an experimental community project (a.k.a "labs"). This means that community members use it and contribute to it, but it is not officially supported or recommended by the ReasonML community. Using this project in a production environment means being willing to contribute yourself if needs be.

Also, the expected lifetime of this PPX is relatively short. OCaml 4.08 has introduced native syntax for doing the same thing that this PPX does (`let+`). At the time of this writing, Bucklescript is still a ways out from supporting OCaml 4.08, or backporting support for `let+`. This PPX has two purposes:

- Provide a temporary solution until Bucklescript and Reason both support `let+`.
- Gague community interest in monadic syntax sugar in general and gather feedback on its usage.

## Compatibility

This package only works with bs-platform 6.x and above. If you're stuck on 5.x take a look at [Jared's original repo](https://github.com/jaredly/let-anything).

## Installation

- `npm install --save-dev bs-let`
- Open up your `bsconfig.json` and add `bs-let/ppx` to your `ppx-flags`. It should look something like this:
  ```json
    {
        ...
        "bsc-flags": [...],
        "ppx-flags": ["bs-let/ppx", ...],
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
    let%Option user = maybeUser;
    // Notice that info isn't an option anymore once we use let%Option!
    let%Option info = user.info;
    let%Option address = info.address;
    let%Option street = address.street;
    Some(street->Js.String.toUpperCase)
};
```

That code is flat, readable, and understandable. Here's an alternative without the syntax sugar:

```reasonml
let getStreet = (maybeUser: option(user)): option(string) => {
  maybeUser->Belt.Option.flatMap(user =>
    user.info
    ->Belt.Option.flatMap(personalInfo =>
        personalInfo.address
        ->Belt.Option.flatMap(address =>
            address.street
            ->Belt.Option.flatMap(street =>
                Some(street->Js.String.toUpperCase)
              )
          )
      )
  );
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

There's a whole lot that can be done with this PPX. It's even possible to go a little crazy and start writing modules that combine monads, like `AsyncOption` that will specifically handle optional values inside of promises. But, in practice, those modules are seldom needed. Don't go too crazy, keeping it simple will get you a long, long way.

Things to remember:

- You don't have to name your module anything special. It could be named `Foo` and you can `let%Foo blah = ...`.
- Simple is better than complex.
- Obvious is usually better than hidden.

## About Performance

It's worth noting that this PPX simply produces a _function callback structure_. Why is this important? There are potential performance gains in situations where avoiding a callback structure is possible.

For example, this handrwitten code, which is pretty much what the PPX produces:

```reasonml
let getStreet = (maybeUser: option(user)): option(string) => {
  maybeUser->Belt.Option.flatMap(user =>
    user.info
    ->Belt.Option.flatMap(personalInfo =>
        personalInfo.address
        ->Belt.Option.flatMap(address =>
            address.street
            ->Belt.Option.flatMap(street =>
                Some(street->Js.String.toUpperCase)
              )
          )
      )
  );
};
```

Is _functionally_ equivalent, but inferior in terms of performance, to the following hand-written code:

```reasonml
let getStreetExplicit = (maybeUser: option(user)): option(string) => {
  switch (maybeUser) {
  | None => None
  | Some(user) =>
    switch (user.info) {
    | None => None
    | Some(personalInfo) =>
      switch (personalInfo.address) {
      | None => None
      | Some(address) =>
        switch (address.street) {
        | None => None
        | Some(street) => Some(street->Js.String.toUpperCase)
        }
      }
    }
  };
};
```

Because we're working with Options, we can `switch` on the values instead of `flatMap`-ing. The generated Javascript of the second approach looks like this:

```javascript
function getStreetExplicit(maybeUser) {
  if (maybeUser !== undefined) {
    var match = maybeUser[/* info */ 0];
    if (match !== undefined) {
      var match$1 = match[/* address */ 0];
      if (match$1 !== undefined) {
        var match$2 = match$1[/* street */ 0];
        if (match$2 !== undefined) {
          return match$2.toUpperCase();
        } else {
          return;
        }
      } else {
        return;
      }
    } else {
      return;
    }
  }
}
```

Only one total function invocation is produced by the compiler in this case instead of one invocation _for every bind_. This is significantly faster to execute and may be worth choosing if this function will be very highly trafficked.

In summary, this PPX is not designed to produce the most performant code in every case. It's just designed to make callbacks easier to use.

## Notes

**A Note about Native VS Bucklescript**
This is specifically designed to be helpful with writing Javascript code through ReasonML and Bucklescript. Native OCaml 4.08 implemented a native monadic sugar syntax. So if you're writing native code, I'd suggest skipping this PPX and waiting until [this PR](https://github.com/facebook/reason/pull/2487) lands in Reason, and then adopting the new syntax.

**A Note about Windows**
Currently this project only precomiles binaries for linux and OS X according to the needs of existing maintainers. If you're a Windows user and would like to use this PPX, We'd love a pull-request that moves the project from Travis to Azure Pipelines and builds for all three platforms.

## Credit

This PPX was created by @jaredly and upgraded to the latest OCaml by @anmonteiro. Murphy Randle has merged Antonio's changes to upgrade the package for Bucklescript 6.x and 7.x, re-packaged it to build with `esy` and precompiled binaries for osx and linux. Murphy has also written this readme to describe the most common use-case for this PPX. More features are available but undocumented in this readme. You can see them here: https://github.com/jaredly/let-anything.

- Lovely readme artwork https://icons8.com/ouch/illustration/eastwood-page-under-construction
