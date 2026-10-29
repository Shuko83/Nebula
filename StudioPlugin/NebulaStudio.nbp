{
    "version": 1,
    "description": "NebulaStudio, assembled from the components of StudioPlugin: start it to run NebulaStudio",
    "plugins": [
        {
            "id": "FabienDG.Nebula.StudioPlugin",
            "file": "plugins/StudioPlugin.nbdl"
        }
    ],
    "components": [
        {
            "name": "Studio",
            "type": "FabienDG.Nebula.StudioPlugin.StudioComponent",
            "x": 1310,
            "y": 1300,
            "properties": {}
        },
        {
            "name": "Edition",
            "type": "FabienDG.Nebula.StudioPlugin.EditionZoneComponent",
            "x": 1310,
            "y": 1480,
            "properties": {}
        },
        {
            "name": "Project",
            "type": "FabienDG.Nebula.StudioPlugin.ProjectComponent",
            "x": 1310,
            "y": 1740,
            "properties": {}
        },
        {
            "name": "Plugins",
            "type": "FabienDG.Nebula.StudioPlugin.PluginListComponent",
            "x": 1630,
            "y": 1300,
            "properties": {}
        },
        {
            "name": "Galaxies",
            "type": "FabienDG.Nebula.StudioPlugin.GalaxyListComponent",
            "x": 1630,
            "y": 1460,
            "properties": {
                "directory": ""
            }
        },
        {
            "name": "Components",
            "type": "FabienDG.Nebula.StudioPlugin.ComponentListComponent",
            "x": 1630,
            "y": 1620,
            "properties": {}
        },
        {
            "name": "Properties",
            "type": "FabienDG.Nebula.StudioPlugin.PropertyEditorComponent",
            "x": 1630,
            "y": 1760,
            "properties": {}
        },
        {
            "name": "Problems",
            "type": "FabienDG.Nebula.StudioPlugin.ProblemListComponent",
            "x": 1630,
            "y": 1920,
            "properties": {}
        },
        {
            "name": "Sources",
            "type": "FabienDG.Nebula.StudioPlugin.TabsComponent",
            "x": 1950,
            "y": 1320,
            "properties": {
                "documentMode": true
            }
        },
        {
            "name": "ComponentsGroup",
            "type": "FabienDG.Nebula.StudioPlugin.GroupComponent",
            "x": 1950,
            "y": 1620,
            "properties": {
                "title": "Components"
            }
        },
        {
            "name": "EditionColumn",
            "type": "FabienDG.Nebula.StudioPlugin.SplitterComponent",
            "x": 1950,
            "y": 1840,
            "properties": {
                "vertical": true,
                "sizes": "480, 200, 120",
                "stretched": 0
            }
        },
        {
            "name": "ApplicationGroup",
            "type": "FabienDG.Nebula.StudioPlugin.GroupComponent",
            "x": 2250,
            "y": 1840,
            "properties": {
                "title": "Application"
            }
        },
        {
            "name": "Columns",
            "type": "FabienDG.Nebula.StudioPlugin.SplitterComponent",
            "x": 2550,
            "y": 1580,
            "properties": {
                "vertical": false,
                "sizes": "320, 760, 360",
                "stretched": 1
            }
        },
        {
            "name": "Window",
            "type": "FabienDG.Nebula.StudioPlugin.StudioWindowComponent",
            "x": 2850,
            "y": 1580,
            "properties": {
                "width": 1440,
                "height": 800
            }
        }
    ],
    "bindings": [
        {
            "consumer": "Edition",
            "consumed": "studio",
            "provider": "Studio",
            "provided": "studio"
        },
        {
            "consumer": "Project",
            "consumed": "studio",
            "provider": "Studio",
            "provided": "studio"
        },
        {
            "consumer": "Project",
            "consumed": "zone",
            "provider": "Edition",
            "provided": "zone"
        },
        {
            "consumer": "Plugins",
            "consumed": "studio",
            "provider": "Studio",
            "provided": "studio"
        },
        {
            "consumer": "Plugins",
            "consumed": "zone",
            "provider": "Edition",
            "provided": "zone"
        },
        {
            "consumer": "Galaxies",
            "consumed": "studio",
            "provider": "Studio",
            "provided": "studio"
        },
        {
            "consumer": "Galaxies",
            "consumed": "project",
            "provider": "Project",
            "provided": "project"
        },
        {
            "consumer": "Components",
            "consumed": "studio",
            "provider": "Studio",
            "provided": "studio"
        },
        {
            "consumer": "Properties",
            "consumed": "studio",
            "provider": "Studio",
            "provided": "studio"
        },
        {
            "consumer": "Properties",
            "consumed": "zone",
            "provider": "Edition",
            "provided": "zone"
        },
        {
            "consumer": "Problems",
            "consumed": "studio",
            "provider": "Studio",
            "provided": "studio"
        },
        {
            "consumer": "Problems",
            "consumed": "zone",
            "provider": "Edition",
            "provided": "zone"
        },
        {
            "consumer": "Sources",
            "consumed": "pages",
            "provider": "Plugins",
            "provided": "widget"
        },
        {
            "consumer": "Sources",
            "consumed": "pages",
            "provider": "Galaxies",
            "provided": "widget"
        },
        {
            "consumer": "ComponentsGroup",
            "consumed": "content",
            "provider": "Components",
            "provided": "widget"
        },
        {
            "consumer": "EditionColumn",
            "consumed": "widgets",
            "provider": "Edition",
            "provided": "widget"
        },
        {
            "consumer": "EditionColumn",
            "consumed": "widgets",
            "provider": "Properties",
            "provided": "widget"
        },
        {
            "consumer": "EditionColumn",
            "consumed": "widgets",
            "provider": "Problems",
            "provided": "widget"
        },
        {
            "consumer": "ApplicationGroup",
            "consumed": "content",
            "provider": "EditionColumn",
            "provided": "widget"
        },
        {
            "consumer": "Columns",
            "consumed": "widgets",
            "provider": "Sources",
            "provided": "widget"
        },
        {
            "consumer": "Columns",
            "consumed": "widgets",
            "provider": "ApplicationGroup",
            "provided": "widget"
        },
        {
            "consumer": "Columns",
            "consumed": "widgets",
            "provider": "ComponentsGroup",
            "provided": "widget"
        },
        {
            "consumer": "Window",
            "consumed": "centralWidget",
            "provider": "Columns",
            "provided": "widget"
        },
        {
            "consumer": "Window",
            "consumed": "menus",
            "provider": "Project",
            "provided": "fileMenu"
        },
        {
            "consumer": "Window",
            "consumed": "menus",
            "provider": "Edition",
            "provided": "viewMenu"
        },
        {
            "consumer": "Window",
            "consumed": "menus",
            "provider": "Edition",
            "provided": "runMenu"
        },
        {
            "consumer": "Window",
            "consumed": "statusWidgets",
            "provider": "Edition",
            "provided": "zoomLabel"
        },
        {
            "consumer": "Window",
            "consumed": "studio",
            "provider": "Studio",
            "provided": "studio"
        },
        {
            "consumer": "Window",
            "consumed": "project",
            "provider": "Project",
            "provided": "project"
        }
    ],
    "zones": [
        {
            "title": "What the parts share",
            "color": "#7c3aed",
            "x": 1190,
            "y": 1220,
            "width": 260,
            "height": 700
        },
        {
            "title": "Parts",
            "color": "#3b82f6",
            "x": 1510,
            "y": 1220,
            "width": 260,
            "height": 860
        },
        {
            "title": "Layout",
            "color": "#ec4899",
            "x": 1830,
            "y": 1220,
            "width": 880,
            "height": 860
        },
        {
            "title": "Window",
            "color": "#06b6d4",
            "x": 2750,
            "y": 1480,
            "width": 200,
            "height": 240
        }
    ]
}
