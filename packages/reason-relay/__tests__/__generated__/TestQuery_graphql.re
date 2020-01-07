/* @generated */

module Types = {
  type node = {
    id: string,
    firstName: string,
    onlineStatus: option(SchemaAssets.Enum_OnlineStatus.t),
  };
  type edges = {node: option(node)};
  type users = {edges: option(array(option(edges)))};
};

open Types;

type response = {users: option(users)};
type refetchVariables = {status: option(SchemaAssets.Enum_OnlineStatus.t)};
let makeRefetchVariables = (~status=?, ()): refetchVariables => {
  status: status,
};
type variables = {status: option(SchemaAssets.Enum_OnlineStatus.t)};

module FragmentConverters: {} = {};

module Internal = {
  type responseRaw;
  let responseConverter: Js.Dict.t(array((int, string))) = [%raw
    {| {"users":[[0,""]],"users_edges":[[0,""],[1,""]],"users_edges_node":[[0,""]],"users_edges_node_onlineStatus":[[0,""],[2,"enum_OnlineStatus"]]} |}
  ];
  let responseConverterMap = {
    "enum_OnlineStatus": SchemaAssets.Enum_OnlineStatus.unwrap,
  };
  let convertResponse = v =>
    v
    ->ReasonRelay._convertObj(
        responseConverter,
        responseConverterMap,
        Js.undefined,
      );

  let variablesConverter: Js.Dict.t(array((int, string))) = [%raw
    {| {"status":[[0,""],[2,"enum_OnlineStatus"]]} |}
  ];
  let variablesConverterMap = {
    "enum_OnlineStatus": SchemaAssets.Enum_OnlineStatus.wrap,
  };
  let convertVariables = v =>
    v
    ->ReasonRelay._convertObj(
        variablesConverter,
        variablesConverterMap,
        Js.undefined,
      );
};

type operationType = ReasonRelay.queryNode;

let node: operationType = [%bs.raw
  {| (function(){
var v0 = [
  {
    "kind": "LocalArgument",
    "name": "status",
    "type": "OnlineStatus",
    "defaultValue": null
  }
],
v1 = [
  {
    "kind": "LinkedField",
    "alias": null,
    "name": "users",
    "storageKey": null,
    "args": [
      {
        "kind": "Variable",
        "name": "status",
        "variableName": "status"
      }
    ],
    "concreteType": "UserConnection",
    "plural": false,
    "selections": [
      {
        "kind": "LinkedField",
        "alias": null,
        "name": "edges",
        "storageKey": null,
        "args": null,
        "concreteType": "UserEdge",
        "plural": true,
        "selections": [
          {
            "kind": "LinkedField",
            "alias": null,
            "name": "node",
            "storageKey": null,
            "args": null,
            "concreteType": "User",
            "plural": false,
            "selections": [
              {
                "kind": "ScalarField",
                "alias": null,
                "name": "id",
                "args": null,
                "storageKey": null
              },
              {
                "kind": "ScalarField",
                "alias": null,
                "name": "firstName",
                "args": null,
                "storageKey": null
              },
              {
                "kind": "ScalarField",
                "alias": null,
                "name": "onlineStatus",
                "args": null,
                "storageKey": null
              }
            ]
          }
        ]
      }
    ]
  }
];
return {
  "kind": "Request",
  "fragment": {
    "kind": "Fragment",
    "name": "TestQuery",
    "type": "Query",
    "metadata": null,
    "argumentDefinitions": (v0/*: any*/),
    "selections": (v1/*: any*/)
  },
  "operation": {
    "kind": "Operation",
    "name": "TestQuery",
    "argumentDefinitions": (v0/*: any*/),
    "selections": (v1/*: any*/)
  },
  "params": {
    "operationKind": "query",
    "name": "TestQuery",
    "id": null,
    "text": "query TestQuery(\n  $status: OnlineStatus\n) {\n  users(status: $status) {\n    edges {\n      node {\n        id\n        firstName\n        onlineStatus\n      }\n    }\n  }\n}\n",
    "metadata": {}
  }
};
})() |}
];
