{
    "version": 1,
    "description": "NebulaStudio, assembled from the components of StudioPlugin: start it to run NebulaStudio",
    "plugins": [
        {
            "id": "FabienDG.Nebula.StandardPlugin",
            "file": "plugins/StandardPlugin.nbdl"
        },
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
            "properties": {
                "file": ""
            }
        },
        {
            "name": "History",
            "type": "FabienDG.Nebula.StandardPlugin.HistoryComponent",
            "x": 1310,
            "y": 1940,
            "properties": {
                "maxSize": 100,
                "undoActive": true,
                "redoActive": true
            }
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
            "name": "Overview",
            "type": "FabienDG.Nebula.StudioPlugin.OverviewComponent",
            "x": 1630,
            "y": 2080,
            "properties": {}
        },
        {
            "name": "Sources",
            "type": "FabienDG.Nebula.StudioPlugin.TabsComponent",
            "x": 1950,
            "y": 1300,
            "properties": {
                "documentMode": true,
                "hiddenWhileDragging": false
            }
        },
        {
            "name": "EditionColumn",
            "type": "FabienDG.Nebula.StudioPlugin.SplitterComponent",
            "x": 1950,
            "y": 1800,
            "properties": {
                "vertical": true,
                "sizes": "620, 120",
                "stretched": 0,
                "hiddenWhileDragging": false
            }
        },
        {
            "name": "SideColumn",
            "type": "FabienDG.Nebula.StudioPlugin.SplitterComponent",
            "x": 2250,
            "y": 1360,
            "properties": {
                "vertical": true,
                "sizes": "330, 240, 180",
                "stretched": 0,
                "hiddenWhileDragging": true
            }
        },
        {
            "name": "Columns",
            "type": "FabienDG.Nebula.StudioPlugin.SplitterComponent",
            "x": 2250,
            "y": 1700,
            "properties": {
                "vertical": false,
                "sizes": "1080, 360",
                "stretched": 0,
                "hiddenWhileDragging": false
            }
        },
        {
            "name": "Window",
            "type": "FabienDG.Nebula.StudioPlugin.StudioWindowComponent",
            "x": 2550,
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
            "consumer": "Edition",
            "consumed": "history",
            "provider": "History",
            "provided": "history"
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
            "consumer": "Overview",
            "consumed": "studio",
            "provider": "Studio",
            "provided": "studio"
        },
        {
            "consumer": "Overview",
            "consumed": "zone",
            "provider": "Edition",
            "provided": "zone"
        },
        {
            "consumer": "Overview",
            "consumed": "viewMenu",
            "provider": "Edition",
            "provided": "viewMenu"
        },
        {
            "consumer": "Sources",
            "consumed": "pages",
            "provider": "Components",
            "provided": "widget"
        },
        {
            "consumer": "Sources",
            "consumed": "pages",
            "provider": "Galaxies",
            "provided": "widget"
        },
        {
            "consumer": "Sources",
            "consumed": "pages",
            "provider": "Plugins",
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
            "provider": "Problems",
            "provided": "widget"
        },
        {
            "consumer": "SideColumn",
            "consumed": "widgets",
            "provider": "Sources",
            "provided": "widget"
        },
        {
            "consumer": "SideColumn",
            "consumed": "widgets",
            "provider": "Properties",
            "provided": "widget"
        },
        {
            "consumer": "SideColumn",
            "consumed": "widgets",
            "provider": "Overview",
            "provided": "widget"
        },
        {
            "consumer": "Columns",
            "consumed": "widgets",
            "provider": "EditionColumn",
            "provided": "widget"
        },
        {
            "consumer": "Columns",
            "consumed": "widgets",
            "provider": "SideColumn",
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
            "provided": "editMenu"
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
            "height": 880
        },
        {
            "title": "Parts",
            "color": "#3b82f6",
            "x": 1510,
            "y": 1220,
            "width": 260,
            "height": 1020
        },
        {
            "title": "Layout",
            "color": "#ec4899",
            "x": 1830,
            "y": 1220,
            "width": 580,
            "height": 860
        },
        {
            "title": "Window",
            "color": "#06b6d4",
            "x": 2450,
            "y": 1480,
            "width": 200,
            "height": 240
        }
    ]
}
