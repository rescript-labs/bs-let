/*
 THIS CODE ORIGINALLY WRITTEN BY JARED FORSYTH. MODIFICATIONS PERFORMED BY
 ANTONIO MONTEIRO. ADDITIONAL MODIFICATIONS MAY HAVE BEEN PERFORMED BY MURPHY
 RANDLE, JOSH ROBERTSON, OR OTHER MEMEBERS OF THE BLOOM BUILT TEAM.
 */

open Ppxlib;

/*
 * https://ocsigen.org/lwt/dev/api/Ppx_lwt
 * https://github.com/zepalmer/ocaml-monadic
 */
let fail = (loc, txt) => Location.raise_errorf(~loc, txt);

let mkloc = (txt, loc) => {
  {Location.txt, loc};
};

let lid_last = fun
  | Lident(s) => s
  | Ldot(_, s) => s
  | Lapply(_, _) => failwith("lid_last on functor application")


let rec process_bindings = (bindings, ident) =>
  Parsetree.(
    switch (bindings) {
    | [] => assert(false)
    | [binding] => (binding.pvb_pat, binding.pvb_expr)
    | [binding, ...rest] =>
      let (pattern, expr) = process_bindings(rest, ident);
      (
        Ast_helper.Pat.tuple(
          ~loc=binding.pvb_loc,
          [binding.pvb_pat, pattern],
        ),
        Ast_helper.Exp.apply(
          ~loc=binding.pvb_loc,
          Ast_helper.Exp.ident(
            ~loc=binding.pvb_loc,
            mkloc(Longident.Ldot(ident, "and_"), binding.pvb_loc),
          ),
          [(Nolabel, binding.pvb_expr), (Nolabel, expr)],
        ),
      );
    }
  );

let parseLongident = txt => {
  let parts = Str.split(Str.regexp_string("."), txt);
  let rec loop = (current, parts) =>
    switch (current, parts) {
    | (None, []) => assert(false)
    | (Some(c), []) => c
    | (None, [one, ...more]) => loop(Some(Longident.Lident(one)), more)
    | (Some(c), [one, ...more]) =>
      loop(Some(Longident.Ldot(c, one)), more)
    };
  loop(None, parts);
};

class mapper = {
  as _;
  inherit class Ast_traverse.map as super;

  pub! expression = expr => {
    /* TODO throw error on structure items */
    switch (expr.pexp_desc) {
    | Pexp_extension((
        {txt, loc},
        PStr([
          {
            pstr_desc:
              Pstr_eval(
                {pexp_loc, pexp_desc: Pexp_try(value, handlers), _},
                _attributes,
              ),
            _
          },
        ]),
      )) =>
      let ident = parseLongident(txt);
      let last = lid_last(ident);
      if (last != String.capitalize_ascii(last)) {
        super#expression(expr);
      } else {
        let handlerLocStart = List.hd(handlers).pc_lhs.ppat_loc;
        let handlerLocEnd =
          List.nth(handlers, List.length(handlers) - 1).pc_rhs.pexp_loc;
        let handlerLoc = {...handlerLocStart, loc_end: handlerLocEnd.loc_end};
        let try_ =
          Ast_helper.Exp.ident(
            ~loc=pexp_loc,
            mkloc(Longident.Ldot(ident, "try_"), loc),
          );
        Ast_helper.Exp.apply(
          ~loc,
          try_,
          [
            (Nolabel, super#expression(value)),
            (Nolabel, Ast_helper.Exp.function_(~loc=handlerLoc, handlers)),
          ],
        );
      };
    | Pexp_extension((
        {txt, loc},
        PStr([
          {
            pstr_desc:
              Pstr_eval(
                {pexp_desc: Pexp_let(Nonrecursive, bindings, continuation), _},
                _attributes,
              ),
            _
          },
        ]),
      )) =>
      let ident = parseLongident(txt);
      let last = lid_last(ident);
      if (last != String.capitalize_ascii(last)) {
        super#expression(expr);
      } else {
        let (pat, expr) = process_bindings(bindings, ident);
        let let_ =
          Ast_helper.Exp.ident(
            ~loc,
            mkloc(Longident.Ldot(ident, "let_"), loc),
          );
        Ast_helper.Exp.apply(
          ~loc,
          let_,
          [
            (Nolabel, super#expression(expr)),
            (
              Nolabel,
              Ast_helper.Exp.fun_(
                ~loc,
                Nolabel,
                None,
                pat,
                super#expression(continuation),
              ),
            ),
          ],
        );
      };
    | _ => super#expression(expr)
    };
  };
};

let structure_mapper = s => (new mapper)#structure(s);

let () =
  Ppxlib.Driver.register_transformation(
    ~preprocess_impl=structure_mapper,
    "let-anything",
  );
