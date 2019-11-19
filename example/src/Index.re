type exampleRecord = {
  name: string,
  age: int,
};

module R = {
  let let_ = Belt.Result.map;
};

{
  let a = Ok({name: "sal", age: 21});

  let%R b = a;
  b.age + 29;
};