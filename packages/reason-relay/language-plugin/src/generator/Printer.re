open Types;

let printQuoted = propName => "\"" ++ propName ++ "\"";
let printJsTPropName = propName => propName |> printQuoted;
let printRecordPropName = propName => propName;
let printEnumName = name => "enum_" ++ name;
let getObjName = name => "obj_" ++ name;
let printEnumTypeName = name => "SchemaAssets.Enum_" ++ name ++ ".t";
let printWrappedEnumName = name => "SchemaAssets.Enum_" ++ name ++ ".wrapped";
let printEnumUnwrapFnReference = name =>
  "SchemaAssets.Enum_" ++ name ++ ".unwrap";
let printEnumWrapFnReference = name => "SchemaAssets.Enum_" ++ name ++ ".wrap";
let printUnionName = name => "Union_" ++ name;
let printUnionTypeName = name => "Union_" ++ name ++ ".t";
let printUnionUnwrapFnReference = name => "Union_" ++ name ++ ".unwrap";
let printUnionWrapFnReference = name => "Union_" ++ name ++ ".wrap";
let printWrappedUnionName = name => "union_" ++ name ++ "_wrapped";
let printFragmentRef = name => name ++ "_graphql.t";
let getFragmentRefName = name => "__$fragment_ref__" ++ name;
let printAnyType = () => "ReasonRelay.any";

let printScalar = scalarValue =>
  switch (scalarValue) {
  | String => "string"
  | Int => "int"
  | Float => "float"
  | Boolean => "bool"
  | CustomScalar(str) => str
  | Any => printAnyType()
  };

let makeUnionName = path =>
  path |> Tablecloth.List.reverse |> Tablecloth.String.join(~sep="_");

let rec printTypeReference = (~state: option(fullState), typeName: string) =>
  switch (state) {
  | Some(state) =>
    switch (
      state.enums |> Tablecloth.List.find(~f=name => name == typeName),
      state.objects
      |> Tablecloth.List.find(~f=(obj: finalizedObj) =>
           obj.name == Some(typeName)
         ),
    ) {
    | (Some(enumName), _) => printEnumTypeName(enumName)
    | (_, Some(_)) => Tablecloth.String.uncapitalize(typeName)
    | _ => typeName
    }
  | None => typeName
  }
and printPropType = (~propType, ~optType, ~state: Types.fullState) =>
  switch (propType) {
  | Scalar(scalar) => printScalar(scalar)
  | Object(obj) => printObjectOrReference(~obj, ~optType, ~state)
  | ObjectReference(objName) =>
    objName |> getObjName |> printTypeReference(~state=None)
  | Array(propValue) => printArray(~propValue, ~optType, ~state)
  | Enum(name) => printEnumTypeName(name)
  | Union(union) => printUnionTypeName(makeUnionName(union.atPath))
  | FragmentRefValue(name) => printFragmentRef(name)
  | TypeReference(name) => printTypeReference(~state=Some(state), name)
  }
and printPropValue = (~propValue, ~optType, ~state) => {
  let str = ref("");
  let addToStr = s => str := str^ ++ s;

  if (propValue.nullable) {
    switch (optType) {
    | Types.JsNullable => addToStr("Js.Nullable.t(")
    | Option => addToStr("option(")
    };
  };

  printPropType(~propType=propValue.propType, ~optType, ~state) |> addToStr;

  if (propValue.nullable) {
    addToStr(")");
  };

  str^;
}
and printObject = (~obj: object_, ~optType: objectOptionalType, ~state) => {
  switch (
    obj.values
    |> Tablecloth.Array.filter(~f=v =>
         switch (v) {
         | FragmentRef(_) => false
         | Prop(_) => true
         }
       )
    |> Array.length,
    obj.mode,
  ) {
  | (_, OnlyFragmentRefs) =>
    let str = ref("{.");
    let addToStr = s => str := str^ ++ s;

    obj.values
    |> Tablecloth.Array.filter(~f=v =>
         switch (v) {
         | FragmentRef(_) => true
         | Prop(_) => false
         }
       )
    |> Array.iteri((index, p) => {
         if (index > 0) {
           addToStr(",");
         };

         addToStr(
           switch (p) {
           | FragmentRef(name) =>
             (name |> getFragmentRefName |> printQuoted)
             ++ ": "
             ++ printFragmentRef(name)
           | Prop(_) => ""
           },
         );
       });

    addToStr("}");
    str^;
  | (0, _) => "unit"
  | (_, JsT) =>
    let str = ref("{.");
    let addToStr = s => str := str^ ++ s;

    obj.values
    |> Array.iteri((index, p) => {
         if (index > 0) {
           addToStr(",");
         };

         addToStr(
           switch (p) {
           | Prop(name, propValue) =>
             printJsTPropName(name)
             ++ ": "
             ++ printPropValue(~propValue, ~optType, ~state)
           | FragmentRef(name) =>
             (name |> getFragmentRefName |> printQuoted)
             ++ ": "
             ++ printFragmentRef(name)
           },
         );
       });

    addToStr("}");
    str^;
  | (_, Record) =>
    let str = ref("{");
    let addToStr = s => str := str^ ++ s;

    obj.values
    |> Tablecloth.Array.filter(~f=v =>
         switch (v) {
         | FragmentRef(_) => false
         | Prop(_) => true
         }
       )
    |> Array.iteri((index, p) => {
         if (index > 0) {
           addToStr(",");
         };

         addToStr(
           switch (p) {
           | Prop(name, propValue) =>
             printRecordPropName(name)
             ++ ": "
             ++ printPropValue(~propValue, ~optType, ~state)
           // Fragment refs aren't supported in records, they'll need to be converted to JsT before fragments refs can be used
           | FragmentRef(_) => ""
           },
         );
       });

    addToStr("}");
    str^;
  };
}
and printArray = (~propValue, ~optType, ~state) =>
  "array(" ++ printPropValue(~propValue, ~optType, ~state) ++ ")"
and printObjectOrReference = (~state: fullState, ~obj: object_, ~optType) => {
  switch (
    state.objects |> Tablecloth.List.find(~f=o => {o.atPath == obj.atPath})
  ) {
  | Some({typeName: Some(typeName)}) =>
    Tablecloth.String.uncapitalize(typeName)
  | Some(_)
  | None => printObject(~optType, ~obj, ~state)
  };
};

let printRefetchVariablesMaker = (obj: object_, ~state) => {
  let str = ref("");
  let addToStr = s => str := str^ ++ s;

  addToStr("type refetchVariables = ");
  addToStr(
    printObject(
      ~state,
      ~obj={
        atPath: [],
        connection: None,
        mode: Record,
        values:
          obj.values
          |> Array.map(value =>
               switch (value) {
               | Prop(name, {nullable: false} as propValue) =>
                 Prop(name, {...propValue, nullable: true})
               | a => a
               }
             ),
      },
      ~optType=Option,
    ),
  );
  addToStr(";");

  addToStr("let makeRefetchVariables = (");

  obj.values
  |> Array.iteri((index, p) => {
       if (index > 0) {
         addToStr(",");
       };

       addToStr(
         switch (p) {
         | Prop(name, _) => "~" ++ name ++ "=?"
         | FragmentRef(_) => ""
         },
       );
     });

  addToStr(", ()): refetchVariables => {");

  obj.values
  |> Array.iteri((index, p) => {
       if (index > 0) {
         addToStr(",");
       };

       addToStr(
         switch (p) {
         | Prop(name, _) => name ++ ": " ++ name
         | FragmentRef(_) => ""
         },
       );
     });

  addToStr("}");
  str^;
};

let printRootType = (~state: fullState, rootType) =>
  switch (rootType) {
  | Operation(obj) =>
    "type response = " ++ printObject(~obj, ~optType=Option, ~state) ++ ";"
  | Variables(obj) =>
    "type variables = " ++ printObject(~obj, ~optType=Option, ~state) ++ ";"
  | RefetchVariables(obj) =>
    switch (obj.values |> Array.length) {
    | 0 => ""
    | _ => printRefetchVariablesMaker(~state, obj) ++ ";"
    }

  | Fragment(obj) =>
    "type fragment = " ++ printObject(~obj, ~optType=Option, ~state) ++ ";"
  | ObjectTypeDeclaration({name, definition, optType}) =>
    "type "
    ++ Tablecloth.String.uncapitalize(name)
    ++ (
      // Because generated records can be empty if the object only contain fragment
      // references, we'll need to print empty records as abstract types here,
      // in the name of type safety!
      switch (
        definition.values
        |> Tablecloth.Array.filter(~f=v =>
             switch (v) {
             | Prop(_) => true
             | FragmentRef(_) => false
             }
           )
        |> Tablecloth.Array.length
      ) {
      | 0 => ""
      | _ => " = " ++ printObject(~obj=definition, ~optType, ~state)
      }
    )
    ++ ";"
  | PluralFragment(obj) =>
    "type fragment_t = "
    ++ printObject(~obj, ~optType=Option, ~state)
    ++ ";\n"
    ++ "type fragment = array(fragment_t);"
  };

let printFragmentExtractor = (~obj: object_, ~state, ~printMode, name) => {
  let fnName = name ++ "_getFragments";
  let signature =
    name
    ++ " => "
    ++ printObject(
         ~obj={...obj, mode: OnlyFragmentRefs},
         ~optType=JsNullable,
         ~state,
       );

  switch (printMode) {
  | Full => "external " ++ fnName ++ ": " ++ signature ++ " = \"%identity\";"
  | Signature => "let " ++ fnName ++ ": " ++ signature ++ ";"
  };
};

let printObjectDefinition = (~name, ~state, ~printMode, definition) => {
  let str = ref("");
  let addToStr = s => str := str^ ++ s;

  switch (definition.mode) {
  | Record =>
    switch (
      definition.values
      |> Tablecloth.Array.filter(~f=v =>
           switch (v) {
           | FragmentRef(_) => true
           | Prop(_) => false
           }
         )
      |> Tablecloth.Array.length
    ) {
    | 0 => ()
    | _ =>
      addToStr("\n");
      addToStr(
        name |> printFragmentExtractor(~state, ~printMode, ~obj=definition),
      );
    }
  | JsT
  | OnlyFragmentRefs => ()
  };

  str^;
};

let printRootObjectTypeConverters =
    (~state: fullState, ~printMode: objectPrintMode=Full, rootType) => {
  let assets =
    switch (rootType) {
    | Operation(definition) => Some(("response", definition))
    | PluralFragment(definition)
    | Fragment(definition) => Some(("fragment", definition))
    | Variables(definition) => Some(("variables", definition))
    | ObjectTypeDeclaration({name, definition}) => Some((name, definition))
    | RefetchVariables(_) => None
    };

  switch (assets) {
  | Some((name, definition)) =>
    definition |> printObjectDefinition(~name, ~state, ~printMode)
  | None => ""
  };
};

let printUnion = (~state, union: union) => {
  let prefix = "module ";
  let unionName = union.atPath |> makeUnionName |> printUnionName;

  let unwrapUnion = "external __unwrap_union: wrapped => {. \"__typename\": string } = \"%identity\";";

  let typeDefs = ref("type wrapped;\n");
  let addToTypeDefs = Utils.makeAddToStr(typeDefs);

  let unwrappers = ref("");
  let addToUnwrappers = Utils.makeAddToStr(unwrappers);

  let typeT = ref("type t = [");
  let addToTypeT = Utils.makeAddToStr(typeT);

  union.members
  |> List.iter(({name, shape}: Types.unionMember) => {
       let usedRecordNames: ref(list(string)) = ref([]);
       let addUsedRecordName = Utils.makeAddToList(usedRecordNames);
       let unionTypeName = Tablecloth.String.uncapitalize(name);

       let allObjects: list(Types.finalizedObj) =
         shape
         |> Utils.extractNestedObjects
         |> List.map((definition: Types.object_) =>
              {
                name: None,
                typeName: {
                  let name =
                    Utils.findAppropriateObjName(
                      ~prefix=Some(unionTypeName),
                      ~usedRecordNames=usedRecordNames^,
                      ~path=definition.atPath,
                    );

                  addUsedRecordName(name);
                  Some(name);
                },
                atPath: definition.atPath,
                definition,
              }
            );

       let definitions =
         allObjects
         |> Tablecloth.List.map(~f=(definition: Types.finalizedObj) =>
              ObjectTypeDeclaration({
                name:
                  Tablecloth.Option.withDefault(
                    ~default="",
                    definition.typeName,
                  ),
                atPath: definition.atPath,
                definition: definition.definition,
                optType: Option,
              })
            );

       let stateWithUnionDefinitions = {...state, objects: allObjects};

       definitions
       |> Tablecloth.List.iter(~f=definition => {
            addToTypeDefs(
              definition |> printRootType(~state=stateWithUnionDefinitions),
            )
          });

       addToTypeDefs(
         "type "
         ++ unionTypeName
         ++ " = "
         ++ printObject(
              ~obj=Utils.adjustObjectPath(~path=[unionTypeName], shape),
              ~optType=Option,
              ~state=stateWithUnionDefinitions,
            )
         ++ ";",
       );

       addToUnwrappers(
         "external __unwrap_"
         ++ unionTypeName
         ++ ": wrapped => "
         ++ unionTypeName
         ++ " = \"%identity\";",
       );
     });

  union.members
  |> List.iter(({name}: Types.unionMember) =>
       addToTypeT(
         " | `" ++ name ++ "(" ++ Tablecloth.String.uncapitalize(name) ++ ")",
       )
     );

  addToTypeT(" | `UnmappedUnionMember];");

  let unwrapFnImpl =
    ref(
      {|
  let unwrap = wrapped => {
    let unwrappedUnion = wrapped |> __unwrap_union;
    switch (unwrappedUnion##__typename) {
  |},
    );

  let addToUnwrapFnImpl = Utils.makeAddToStr(unwrapFnImpl);

  union.members
  |> List.iter(({name}: Types.unionMember) =>
       addToUnwrapFnImpl(
         "| \""
         ++ name
         ++ "\" => `"
         ++ name
         ++ "(wrapped |> __unwrap_"
         ++ Tablecloth.String.uncapitalize(name)
         ++ ")",
       )
     );

  addToUnwrapFnImpl({|
      | _ => `UnmappedUnionMember
    };
  };
  |});

  prefix
  ++ unionName
  ++ ": {"
  ++ typeDefs^
  ++ typeT^
  ++ "let unwrap: wrapped => t;"
  ++ "} = { "
  ++ typeDefs^
  ++ unwrapUnion
  ++ typeT^
  ++ unwrappers^
  ++ unwrapFnImpl^
  ++ "}";
};

let fragmentRefAssets = (~plural=false, fragmentName) => {
  let fref = fragmentName |> getFragmentRefName;

  let str = ref("");
  let addToStr = s => str := str^ ++ s;

  addToStr("type t;");
  addToStr("type fragmentRef;");
  addToStr("type fragmentRefSelector('a) = ");

  if (plural) {
    addToStr("array(");
  };

  addToStr("{.. \"" ++ fref ++ "\": t} as 'a");

  if (plural) {
    addToStr(")");
  };

  addToStr(";");

  addToStr(
    "external getFragmentRef: fragmentRefSelector('a) => fragmentRef = \"%identity\";",
  );

  str^;
};

let operationType = (operationType: Types.operationType) => {
  let opType =
    switch (operationType) {
    | Fragment(_) => "fragment"
    | Query(_) => "query"
    | Mutation(_) => "mutation"
    | Subscription(_) => "subscription"
    };

  "type operationType = ReasonRelay." ++ opType ++ "Node;";
};

let printType = typeText => {j|type $typeText;|j};

let opaqueUnionType = unions =>
  Tablecloth.(
    unions
    |> List.map(~f=(union: Types.union) =>
         union.atPath |> makeUnionName |> printWrappedUnionName |> printType
       )
    |> String.join(~sep="\n")
  );

[@genType]
let printCode = str => str |> Reason.parseRE |> Reason.printRE;