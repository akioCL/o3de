{ 
    "Source" : "HairRenderingFillPPLL.azsl",	
    "DrawList" : "hairFillPass",

    "CompilerHints" : 
    { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : true,
             "CompareFunc" : "GreaterEqual"     // In TressFX its LessEqual - Atom uses inverse depth
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

    "RasterState" :
    {
        "CullMode" : "None"
    },

    "BlendState" : 
    {
        "Enable" : false
    },

    "ProgramSettings":
    {
        "EntryPoints":
      [
        {
          "name": "RenderHairVS",
          "type": "Vertex"
        },
        {
          "name": "PPLLFillPS",
          "type": "Fragment"
        }
      ]
    }
}
