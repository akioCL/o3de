{
    "Source" : "EditorModeMask.azsl",
 
    //"DepthStencilState" : { 
    //    "Depth" : { "Enable" : true, "CompareFunc" : "GreaterEqual" }
    //},

    "DepthStencilState" : { 
        "Depth" : { "Enable" : false }
    },

    //"DepthStencilState" : { 
    //    "Depth" : { "Enable" : false }
    //},

    "DrawList" : "editormodemask",

    //"RasterState" :
    //{
    //    "depthBias" : "10",
    //    "depthBiasSlopeScale" : "4"
    //},

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MainVS",
          "type": "Vertex"
        },
        {
          "name": "MainPS",
          "type": "Fragment"
        }
      ]
    }    
}