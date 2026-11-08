# Nebula — carte du code

Framework C++/Qt 6 (Core + Widgets) pour assembler des applications à partir de composants fournis par des plugins.
Le [README](README.md) documente l'API publique en détail ; ce fichier résume où sont les choses et les pièges.
À tenir à jour quand l'architecture change.

## Arborescence

| Dossier | Cible | Rôle |
| --- | --- | --- |
| `Core/` | `Nebula::Core` (DLL) | Composants, plugins, fabrique, application, galaxies. En-têtes publics dans `include/Nebula/{Application,Component,Galaxy,Plugin,Settings}/`, sources dans `src/` (module `Galaxy` : `Galaxy`, `GalaxyInfo`, `Stargate` ; ce qu'`Universe` en fait reste dans `Universe.cpp`) |
| `Application/` | `Nebula::Application` (DLL) | Issue #73 : `NebulaApplication`, la `QApplication` d'un exécutable dont le contenu est une galaxy, et `ApplicationOptions` (en-têtes publics dans `include/Nebula/Application/`, à côté de ceux de Core sous le même chemin), et `MessageLog` (privé, `src/`), où vont les messages. Lie `Nebula::Core` et `Qt6::Widgets` en `PUBLIC` |
| `Core/cmake/` | — | `nebula_add_plugin()` et les gabarits `plugin.h.in` / `plugin.json.in`, livrés avec `Core` dans le paquet |
| `Plugins/Standard/` | `Nebula::Standard` (INTERFACE, en-têtes seuls) | Issue #130 : les interfaces des composants standard, dans `include/Nebula/Standard/` (`LogLevel.h`, `ILog.h`, `ILogControl.h`, et issue #132 `Command.h`, `IHistory.h`), qu'un plugin lie pour les consommer ; composant du paquet, `find_package(Nebula COMPONENTS Core Standard)`. Voir Plugins standard |
| `Plugins/StandardPlugin/` | `.nbdl` dans `bin/plugins` | Issue #130 : le premier plugin standard, livré avec Nebula pour toute application, à la version de Nebula : `LogComponent` (fournit `log` : `ILog`, et `logControl` : `ILogControl`), qui pilote pendant l'exécution le log de tout le processus, et `HistoryComponent` (issue #132, fournit `history` : `IHistory`), l'historique de ce qui se défait |
| `NebulaStudio/` | `NebulaStudio` (exe Widgets) | L'éditeur graphique : son `main.cpp` est une `NebulaApplication` qui fait tourner la galaxy de `StudioPlugin/` (issue #73) ; le code de ses parties est dans `NebulaStudio/src/`, sans namespace, celui des fichiers de réglages dans `NebulaStudio/src/Settings/`, celui des galaxies (`GalaxyModel`, `GalaxyViewer`, `GalaxyPanel`) dans `NebulaStudio/src/Galaxy/`, le logo de Nebula, les icônes du glossaire et la police des titres dans `NebulaStudio/resources/` |
| `StudioPlugin/` | `.nbdl` dans `bin/plugins` | Issue #103 : les parties de NebulaStudio, chacune un composant (`src/`, voir NebulaStudio), faites de ses classes, dont les sources sont recompilées dans le plugin (toutes celles de `NebulaStudio/src/` sauf `main.cpp` et `SplashScreen.cpp`, avec `nebulastudio.qrc`), et `NebulaStudio.nbp`, la galaxy qui les assemble en la fenêtre de NebulaStudio, copiée dans `<build>/bin[/<config>]/scene/` à chaque build (cible `StudioGalaxy`) |
| `exemple/ExamplePlugin` | `.nbdl` dans `bin/plugins` | Composant `Greeter` (fournit `greeter` : `IGreeter`) |
| `exemple/WidgetsPlugin` | `.nbdl` dans `bin/plugins` | L'exemple de la bonne pratique (issue #79) : des composants nommés `<Nom>Component`, chacun créant ses objets (membres) dans son constructeur. `MainWindowComponent` (fournit `mainWindow` : la `QMainWindow`, montrée pendant l'exécution ; consomme `centralWidget` : `QWidget`), `TopBarComponent` (une `QToolBar` ; consomme `mainWindow`, `Mandatory`, les listes `actions` : `QAction` et `widgets` : `QWidget`, montrés dans l'ordre de leurs liaisons, issue #67), `ActionComponent` (fournit `action` : `QAction`), `CounterComponent` (consomme `action`, `Mandatory`, compte pendant l'exécution ; fournit `display` : son `QLabel`) et `LabelsComponent` (issue #86 : fournit `label1`… autant de `QWidget` que son `count`, 1 au départ, de 0 à `MaxCount` = 10, chacun le `QLabel` d'un `Label`, objet dont le `text` est édité dans le groupe de son nom ; régler `count` ajoute les suivants ou retire les derniers par `removeProvidedInterface()`) |
| `exemple/ExternalPlugin` | `.nbdl` dans `<build>/external-plugins` | `Ticker` (fournit `display` : `QWidget`, dont il expose les propriétés dans le groupe `Display`), `ProfileSelector` (fournit `display` : la `QComboBox` de ses `profiles`, où l'utilisateur choisit `profile`) et `Window` (consomme `content` : `QWidget`, `Mandatory`) |
| `exemple/ExampleApp` | exe console | Scanne, charge, liste et fait tourner un `Greeter` |
| `exemple/ExampleUniverseApp` | exe console + Widgets | Le `main` type : options de `Universe::addCommandLineOptions()`, `applyCommandLine()`, `open()` du projet passé en argument, puis `start()` |
| `CMakeUtils/` | sous-module git | `cmu_add_target` / `cmu_add_executable` / `cmu_add_shared_library` |
| `cmake/` | — | Issue #139 : `PackageName.cmake`, le nom du paquet (`nebula_package_name()`, `nebula_compiler_name()`, `nebula_visual_studio_year()`, `nebula_is_version_suffix()`), des fonctions de leurs seuls arguments, que `tests/Package/` lance en mode script. Pas livré |
| `docs/release-notes/` | — | Notes de chaque version, `<x.y.z>.md` : le texte de sa release GitHub |
| `CHANGELOG.md` | — | Ce qui change d'une version à l'autre, la dernière en premier, au format Keep a Changelog, en anglais : une section `## [x.y.z] - <date>` par version (Added, Changed, Removed, Fixed) et son lien vers la release en bas |
| `docs/glossary.md` | — | Glossaire Nebula (Nebula, Univers, Galaxy, Astre, Comet, Stargate), en anglais puis en français, référencé par le README : le vocabulaire à employer pour nommer code, documentation et interface ; Nebula et Univers (la classe `Universe`) sont déjà employés, Galaxy pour un fichier de projet (`GalaxyInfo`, onglet **Galaxies**), et Stargate pour ce qu'une galaxy expose (`Stargate`) |
| `tests/` | `Test<Classe>` dans `<build>/tests/bin[/<config>]` | Tests Qt Test, pilotés par les données : `nebula_add_test()` dans `tests/CMakeLists.txt`, `Application/` (`TestNebulaApplication`, `QTEST_APPLESS_MAIN` : chaque test construit sa `NebulaApplication`, l'une après l'autre, avec sa ligne de commande), `Core/` (une par classe, `TestUniverse` avec les morceaux de projet copiés et collés, `TestGalaxy` pour les galaxies posées, leurs interfaces, leurs propriétés exposées, leur édition, leur relecture et leur copier-coller, `TestStargate` pour la Stargate dans l'application qui l'édite) et `NebulaStudio/` (Colors, Theme avec les bords de son style et le schéma choisi, PluginModel, ComponentModel, EditionView, SceneOverview, DragAside, EditionScene pour ses liens et son copier-coller, SceneCommand pour les étapes de son historique, ComponentFinder, PropertyEditor pour ses groupes et son édition et la page d'un fichier de réglages, SettingsDialog, SettingsValueEditor et SettingsTextDialog pour les valeurs et le texte d'un fichier de réglages, suivi sur le disque, GalaxyModel, Preferences, sources recompilées), `StudioPlugin/` (`TestStudioPlugin` : ses composants chargés depuis le plugin, la galaxy de NebulaStudio ouverte, lancée et relue), `StandardPlugin/` (`TestLogComponent` : le `LogComponent` chargé depuis le plugin, consommé par les interfaces de `Nebula::Standard`, et le niveau du processus vu d'une catégorie du test ; `TestHistoryComponent` : ses commandes faites, défaites et refaites, sa taille, ses options et les commandes composées), `exemple/` (`TestWidgetsPlugin` : ses composants chargés depuis le plugin et assemblés), `Package/` (issue #139, pas des exécutables Qt Test : `TestPackageName`, `cmake -P` de `cmake/PackageName.cmake` et du nom de ce build, et sous Windows `TestVersionDetails<Cible>`, un script PowerShell qui lit la ressource de version de `NebulaStudio`, `Core` et `StandardPlugin`), et `Components/`, bibliothèque statique `TestComponents` (`Probe`, `Source`, `Sink`, `Panel`, `Outlets` : autant d'interfaces et d'objets que son `count`) enregistrée dans le processus du test par `registerTestComponents()` sous l'id de plugin `Test`, et `Plugins/`, les plugins de test construits par `nebula_add_test_plugin()` (`nebula_add_plugin()`, puis retirés du paquet) : `FirstPlugin` (`Alpha`) et `ThirdPlugin` (`Gamma`, `Delta`) dans `test-plugins/`, `SecondPlugin` (`Beta`) dans `other-test-plugins/`, à côté des tests mais pas dans leur `plugins/` (`NEBULA_TEST_PLUGINS_DIR`, `NEBULA_OTHER_TEST_PLUGINS_DIR`). `PluginManager`, les plugins d'un projet ou d'une galaxy et `PluginModel` se testent sur eux : seul `TestWidgetsPlugin` dépend d'un exemple, celui qu'il teste (issue #83). Rien n'est installé |
| `docs/testing.md` | — | Lancer, déboguer et écrire les tests, et mesurer ce qu'ils couvrent, en français |
| `sonar-project.properties` | — | Issue #147 : ce qu'analyse SonarQube Cloud, que la CI complète de ce que donne le build (voir Build) |
| `CFmake/` | non suivi, dépôt à part | Ne fait pas partie du build : ne pas y toucher |

## Build

- `cmu_add_*` trouve les sources par `GLOB_RECURSE CONFIGURE_DEPENDS` dans `src/` et `include/` : aucun fichier à
  lister dans les `CMakeLists.txt`. Il génère `<NAME>_export.h` (`CORE_EXPORT`) et `<NAME>_info.h`
  (`CoreInfo::version`, `<Nom>Info::buildDate`).
- `cmu_organization` vaut `FabienDG`, défini avant l'inclusion de CMakeUtils : les ids sont
  `FabienDG.Nebula.<Plugin>.<Composant>` et l'IID `FabienDG.Nebula.IPlugin/2.0`. `cmu_copyright`, défini au même
  endroit, vaut `Copyright (C) 2026 Fabien Di Grande`, celui de la licence, à la place de celui que CMakeUtils
  tirerait de l'organisation : il est écrit dans les fichiers d'infos (`<NAME>_info.h`, ressources Windows) et
  affiché par le splash de NebulaStudio. `Shuko83` reste le compte GitHub : adresse du dépôt, du sous-module
  CMakeUtils, et `organizationDomain` (valeur par défaut de CMakeUtils).
- Version et paquet (issue #139) : `NEBULA_VERSION_SUFFIX` (cache, vide par défaut, `-rc0` : un tiret puis des lettres
  ou des chiffres, sinon la configuration s'arrête) est passé à `cmu_version_suffix` avant l'inclusion de CMakeUtils,
  qui l'ajoute à `PROJECT_VERSION`, la version par défaut de chaque cible (`<NAME>Info::version`, donc le splash et
  `--version` de NebulaStudio, et les détails du fichier), comme `NebulaPlugin.cmake` pour `version` et `coreVersion`
  (le contrôle de compatibilité ne compare que les nombres, `QVersionNumber`). Sous Windows, CMakeUtils écrit dans
  la ressource de version de chaque binaire la description `<nom> (<cmu_platform>, <compilateur>, Qt <version>)`
  (`win-x64`, `MSVC 19.44.35211.0`, `6.12.0`) et les quatre nombres de la version. Les binaires gardent leur nom.
  `NEBULA_PACKAGE_NAME` (cache interne), calculé après `find_package(Qt6)` par `cmake/PackageName.cmake` :
  `Nebula_<version>_<cmu_platform>_<compilateur>_Qt<majeure>.<mineure>` (`Nebula_2.2.0_win-x64_msvc2022_Qt6.12`,
  `msvc` et l'année de Visual Studio tirée de `MSVC_VERSION`, sinon l'id et la majeure : `gcc14`), que `release.yml`
  lit dans `CMakeCache.txt` pour nommer le zip et son dossier.
- Sorties : `<build>/bin[/<config>]`, plugins dans `plugins/` à côté. Extension `.nbdl`, seule scannée.
- `add_dependencies(NebulaStudio ExamplePlugin WidgetsPlugin ExternalPlugin StudioPlugin StudioGalaxy StandardPlugin)`,
  de même pour `ExampleUniverseApp` (et `ExampleApp` pour ceux de `plugins/`), dans `exemple/CMakeLists.txt` :
  construire l'un reconstruit les plugins du dépôt, et recopie la galaxy de NebulaStudio dans `scene/`.
- Tests : `tests/`, un exécutable Qt Test par classe, lancés par `ctest -C <config>` depuis le build (option
  `NEBULA_BUILD_TESTS`, `ON` par défaut), voir [docs/testing.md](docs/testing.md). CI : Windows, MSVC 2022, Qt 6.12
  (la dernière 6.12.x, issue #141 : la CI ne vérifie plus aucune version antérieure), Release, puis `ctest`. Qt y est
  installé par son installateur en ligne officiel (`use-official` d'install-qt-action), avec le compte Qt des secrets
  `QT_EMAIL` et `QT_PASSWORD` du dépôt, sans lesquels la CI s'arrête (une PR depuis un fork n'y a pas accès) :
  aqtinstall 3.3.0, sa dernière version publiée, ne trouve pas Qt 6.11 et plus, rangées par architecture dans le dépôt
  de Qt ; à retirer quand une version publiée le fera.
  Analyse SonarQube Cloud (issue #147), dans le même job : `sonar-project.properties` (organisation `shuko83`, projet
  `Shuko83_Nebula`, `sonar.sources` = `Core`, `Application`, `Plugins`, `NebulaStudio`, `StudioPlugin`, `exemple`,
  `sonar.tests` = `tests`) ; Release compilé avec les PDB (`/Zi`, `/DEBUG /OPT:REF /OPT:ICF`, sur la ligne de
  `cmake`, pas dans les `CMakeLists.txt` : le paquet de `release.yml` n'en a pas), build sous le build-wrapper de
  SonarQube (`compile_commands.json` dans `build-wrapper/`), puis `ctest` lancé par OpenCppCoverage (`choco`,
  `--cover_children`, qui rend le code de sortie des tests), qui écrit `coverage.xml` en Cobertura
  (`sonar.cfamily.cobertura.reportPaths`) sans les tests, `build/` ni CMakeUtils ; `sonar.projectVersion` =
  `CMAKE_PROJECT_VERSION` du cache, d'où part le nouveau code de `main`. `sonar.qualitygate.wait=true` : l'étape, donc
  la CI, échoue quand le nouveau code ne passe pas la quality gate, Sonar way, la seule du plan Free (notes A en
  sécurité, fiabilité et maintenabilité, 80 % couvert, 3 % dupliqué au plus, hotspots revus). Le token est le secret
  `SONAR_TOKEN`, vérifié avant le build ; l'analyse automatique de SonarQube Cloud est désactivée (elle ferait échouer
  celle de la CI).
  `release.yml` fait la même compilation sur un tag de version (`0.2.0`) et publie la release (voir Process) ; sur une PR qui le modifie, il s'exécute sans rien publier et dépose le zip
  en artefact. Il ajoute au paquet la licence de Space Grotesk à côté de chaque application (NebulaStudio.exe et
  `StudioPlugin.nbdl` la contiennent) et la galaxy de NebulaStudio dans `scene/` de NebulaStudio et d'ExampleUniverseApp.
- C++ récent requis (concepts, `std::ranges`, initialiseurs désignés, `std::type_identity_t`).
- Paquet : CMakeUtils installe chaque target dans `<préfixe>/<target>/` (mode `TARGET`) et copie le résultat dans
  `<build>/package` après chaque build. `cmu_add_cmake_files(TARGET Core …)` y livre `NebulaPlugin.cmake`,
  `Template/` et CMakeUtils dans `Core/cmake/`, que `CoreConfig.cmake` inclut : un projet tiers n'a besoin que de
  `find_package(Nebula COMPONENTS Core)` pour appeler `nebula_add_plugin()`, et de `COMPONENTS Application` pour
  `NebulaApplication`, dont la config inclut celle de Core (`Application.dll` est copiée à côté de chaque exécutable qui
  la lie).

## Core

**Plugin** (`Plugin.h`, `IPlugin.h`, `PluginManager.*`)
- Un plugin est la fabrique de ses composants (issue #94) : `IPlugin` = `buildDate()`, `components()` (leurs
  `QMetaObject`, dans l'ordre d'enregistrement) et `create(nom)` (`nom` = `componentName()` de `ComponentInfo.h`, le
  nom de la classe sans namespace ; `nullptr` sinon).
- `nebula_add_plugin(NAME …)` crée une bibliothèque MODULE et génère la classe du plugin (issue #85), `<Nom>` dans
  `<NAME>.h` (gabarit `plugin.h.in`), dans `<build>/…/<NAME>_nebula/`, dossier ajouté aux includes du plugin : c'est
  aussi son objet racine, `QObject` dérivant de `Nebula::Plugin<<Nom>>`, avec `Q_PLUGIN_METADATA` (le JSON id / name /
  description / version / coreVersion) et `buildDate()`. Le plugin n'écrit que ses composants, qui l'incluent pour s'y
  enregistrer. `NEBULA_PLUGIN` n'est plus qu'un `static_assert(false, …)` qui dit à un plugin écrit pour 1.0.0 de
  retirer sa classe, après avoir déplacé dans le constructeur de ses composants ce que créaient ses `build()` (ceux d'un
  plugin d'avant 1.0.0). Elle ne connaît pas les objets de ses composants : un composant les crée lui-même dans son
  constructeur (issue #79). Le contrôle qui refusait une classe ayant un `build(<Composant>&)` est retiré (issue #88) :
  la classe générée n'en a pas.
- `Nebula::Plugin<P>` (`Plugin.h`, CRTP, tout dans l'en-tête) implémente `components()` et `create()` depuis
  `registered()`, la liste statique de `P` : `NEBULA_REGISTER_COMPONENT(ClassePlugin, Composant);` est une
  initialisation statique qui appelle `Plugin<ClassePlugin>::add<Composant>()` (série de `static_assert`,
  `std::derived_from` pour exiger un héritage public de `Component` et de `Plugin<ClassePlugin>`), au
  chargement de la DLL, avant que Qt crée l'objet racine. Une seule liste, que rien ne recopie.
- `Plugin::Instance<T>` est ce que `create()` instancie : son destructeur appelle `releaseInterfaces()` avant celui
  de `T`. Rien n'empêche plus de compiler `new Greeter` (`Component::builtByPlugin()` est retirée) : la règle
  « un composant se crée par son plugin » est documentée.
- `PluginManager` : `scan(dir)` / `addFile(path)` lisent les métadonnées sans charger (contrôle de l'IID, puis même
  majeure de Core et version ≤ celle de Core) ; `load(id)` charge, lit la date de build et appelle
  `ComponentFactory::instance().add()` avec l'objet racine ; `unload(id)` appelle `ComponentFactory::remove()` puis
  `QPluginLoader::unload()`, qui détruit l'objet racine. La DLL, elle, reste en mémoire
  (voir Pièges) : recharger recrée l'objet racine sans relire le fichier. Erreurs dans `errorString()`.
- RAII : le destructeur décharge ce que ce gestionnaire a chargé (`Entry::loaded`). Il doit donc survivre aux
  composants de ses plugins : dans `Universe`, `m_plugins` est déclaré avant `m_components`. Chaque
  gestionnaire prend sa part du plugin par `QPluginLoader::load()`, que Qt compte : un plugin chargé par deux
  gestionnaires reste tant que l'un des deux le garde, et `unload()` renvoie alors `false`.

**Composant** (`Component.*`, `IComponent.h`, `ComponentFactory.*`, `ComponentInfo.h`, `InterfaceInfo.h`)
- `ComponentFactory` (singleton `instance()`) : l'index des composants des plugins chargés, id → `IPlugin*`
  (`add(pluginId, IPlugin&)`) ; `components()`, `component(id)`, `create(id)` qui le fait créer par son plugin
  (`IPlugin::create()`) et fixe `m_componentId`, `remove(pluginId)` qui oublie les composants d'un plugin.
  `ComponentInfo` est lu depuis le `QMetaObject` : `Q_CLASSINFO` `description` / `category`, et les propriétés à partir de `Component::staticMetaObject.propertyCount()`.
- Deux modes : `Editing` (à la création : propriétés réglées, interfaces liées) → `start()` → `Running` →
  `stop()` → `Editing`. Crochets `onStart()` (peut refuser) / `onStop()`. Signal `stateChanged`.
- Interfaces, typées par `QMetaType::fromType<T*>()`, pointeurs stockés en `void*` :
  - `provideInterface<T>(nom[, pointeur])` (protégé) : déclare, fournit, ou retire avec `nullptr`. Remplacer émet
    `interfaceUnavailable` puis `interfaceAvailable`.
  - `consumeInterface<T>(nom, membre[, InterfaceRequirement], onAvailable, onUnavailable)` (protégé) : le membre est
    écrit par Core. `Mandatory` (`Optional` sinon) dit seulement que le composant ne fonctionne pas sans elle : rien
    ne le vérifie, NebulaStudio le montre.
  - `consumeInterfaceList<T>(nom, QList<T*>& membre[, InterfaceRequirement], onInserted(T*), onRemoved(T*))`
    (protégé, issue #67) : une liste, liée à plusieurs fournies ; le membre tient les pointeurs disponibles dans l'ordre
    des liaisons, réécrit en entier par Core (`writeMember()`) après `onInserted` et avant `onRemoved` (le pointeur y
    est encore). `Mandatory` = au moins une liaison. En interne, toute consommée a une `QList<Bound>` (une au plus hors
    liste), chaque `Bound {provider, providedName, connections, pointer}` suivant son fournisseur
    (`followProvider()`, `deliverInterface()` / `withdrawInterface()` par fournisseur et nom fourni) ; `consume()`
    (privé, celui de `Galaxy` et `Stargate`) enveloppe une interface seule dans ce modèle.
  - `bindInterface(nom, fournisseur, nomFourni)` / `unbindInterface(nom)` (publics) : même type exigé, un consommateur
    n'a qu'un fournisseur, un fournisseur peut avoir plusieurs consommateurs ; une liste ajoute la liaison après les
    autres et refuse une liaison qu'elle a déjà, `unbindInterface(nom)` les défait toutes (la dernière d'abord) et
    `unbindInterface(nom, fournisseur, nomFourni)` une seule.
  - `providedInterfaces()` / `consumedInterfaces()` renvoient des `InterfaceInfo {name, type, available, requirement,
    list}` (`available` : au moins un pointeur pour une liste) ; `interfacesChanged()` est émis à chaque déclaration,
    liaison, déliaison ou changement de disponibilité.
  - `binding(nom)` dit à quoi une interface consommée est liée : `InterfaceBinding {provider, providedName}` (la
    première d'une liste) ; `bindings(nom)` les donne toutes, dans l'ordre. Ce qui lit les liens (enregistrement,
    relecture d'une galaxy, scène, `StargateDialog`) passe par `bindings()`.
  - `unboundMandatoryInterfaces()` : les consommées `Mandatory` liées à rien. Vide = composant opérationnel ; c'est ce
    que NebulaStudio signale (liée suffit, même si le fournisseur n'a pas encore de pointeur, comme `Greeter` en
    édition).
  - `removeProvidedInterface(nom)` / `removeConsumedInterface(nom)` (protégés, issue #86, appelés aussi par `Galaxy`
    et `Stargate`) : ce qu'un composant ne fournit ou ne consomme plus quand une de ses propriétés change (ses widgets,
    autant que son `count`). Une fournie est rendue indisponible (ses consommateurs la lâchent pendant que le pointeur
    vaut encore), oubliée, puis `interfaceRemoved(nom)` délie ses consommateurs (une liste garde ses autres liaisons) ;
    une consommée est déliée (`onUnavailable`) puis oubliée. Rien pour un nom inconnu de ce côté.
  - Privés, pour `Galaxy` et `Stargate` (amies) : `provide()` / `consume()` d'un type connu à l'exécution seulement,
    `renameProvidedInterface()` / `renameConsumedInterface()`, et `releaseInterfaces()`. Signal
    `interfaceRenamed(nom, nouveau)` (une fournie : ses consommateurs suivent son nom, `followProvider()`). Un lien
    connaît le nom fourni au moment où le fournisseur parle, pas à sa création.
- Objets à propriétés (`PropertyObject.*`) : `addPropertiesForQObject(objet[, nom])` (protégé, appelé par le composant
  à sa construction, ou quand une propriété dont ils dépendent est réglée) ajoute un `PropertyObject {object, name}` à
  `propertyObjects()` ; il en sort seul à sa destruction. Signal `propertyObjectsChanged`. `PropertyObject::properties()` : les propriétés déclarées, sans
  `objectName` ni celles `DESIGNABLE false` / `STORED false` ; c'est la liste qu'éditent NebulaStudio et qu'écrit un
  projet.
- Journalisation : catégories `nebula.components` et `nebula.plugins`.

**Application** (`Application/Universe.*`)
- `Universe` (QObject) : l'application assemblée, utilisée telle quelle par NebulaStudio et par le `main` d'une
  application. Possède un `PluginManager` (`plugins()`, scanne `plugins/` à la construction) et les instances
  (`unique_ptr<Component>`), nommées `Greeter1`, `Greeter2`… dans `objectName`. `rename(instance, nom)` renomme en
  gardant les noms uniques et non vides : c'est par son nom qu'un projet et `component(nom)` désignent une instance.
  `remove(instance)` la détruit ; ses liens tombent à sa destruction (`releaseInterfaces()` pour ce
  qu'elle consomme, signal `destroyed` pour ceux qui consomment ce qu'elle fournit). `remove()` et `clear()` la
  sortent de `components()` avant de la détruire, pour que ce que sa destruction fait émettre voie une liste cohérente.
  Signal `componentsChanged` après `instantiate()`, `remove()` et `clear()` (s'il y avait des instances).
- Projet : `read(fichier, erreur)` statique (lecture + contrôle de version), `load(json)` qui renvoie la liste de ce
  qui n'a pas pu être restauré, `open(fichier)` = les deux, strict (`false` au moindre problème), `save()` qui écrit
  `version`, `description` (si elle n'est pas vide), `plugins`, `components` (nom, type, propriétés inscriptibles qui font l'aller-retour JSON, et `objects` : celles de ses
  `propertyObjects()`, retrouvés à l'ouverture par nom puis par ordre d'ajout) et `bindings`. `description()` /
  `setDescription()` : celle du projet, remplacée par `load()`, vidée par `clear()`. Les propriétés sont rendues dans
  l'ordre de leur déclaration, pas dans celui du JSON, alphabétique (`ProjectJson::writeProperties()`, issue #86),
  puis celles des objets, puis les liens : une propriété qui fait les interfaces et les objets d'un composant (son
  `count`) passe avant ce qui en dépend. De même pour les valeurs exposées d'une galaxy, dans l'ordre où elle les
  expose (`setExposedValues()`), chacune retrouvée au moment de l'écrire (`writeExposed()`) : la précédente a pu
  détruire son objet.
- Morceau de projet (issue #136, le copier-coller de NebulaStudio) : `saveInstances(instances)` écrit `plugins`,
  `components` et `bindings` (pas de `version`) de quelques instances, celles du même endroit que la première qu'il
  connaît (`savedInstances()` : dans l'ordre d'instanciation, sans les stargates), une galaxy avec son fichier entier
  (`componentsJson(…, {})`), les liens entre elles seulement (`bindingsJson()`, par `bindingsAmong()`, ne garde que ceux dont le fournisseur
  est parmi les composants écrits, ce que vérifie aussi `save()`) ; vide pendant l'exécution. `insertInstances(json,
  galaxy, problèmes)` : `placingScope()` (partagé avec `instantiateGalaxy()`, ses deux refus en arguments), plugins par
  id puis chargés (`loadInstancePlugins()`, `firstUnloadable()` partagé avec `loadPlugins()`), noms libres
  (`freelyNamed()` : `freeNames()` donne à chacun le sien s'il est libre dans la scope et parmi ceux déjà donnés, sinon
  sa base sans le nombre final et le premier nombre libre après le sien, 2 sans nombre, `derivedName()` ; composants
  et liens renommés, stargates et liens vers d'autres retirés), puis `insert()` : `loadScope()` comme un projet, les
  nouveaux en fin de `scopeComponents()` ; s'il en manque un (plugin qui ne le fournit pas, fichier illisible, cycle,
  même plus bas, `isWhole()`), `withdraw()` retire ceux posés, le dernier d'abord, et `errorString()` dit les problèmes ;
  `settleInserted()` vide `errorString()` et dit les fichiers de réglages d'une galaxy posée dans une galaxy. Renvoie les instances dans l'ordre
  du JSON ; `problems` reçoit le reste (une propriété). Rien ne tient leurs propriétés dans un fichier de réglages.
- Galaxy : un fichier de projet, nommé d'après son fichier. `readGalaxy(fichier, erreur)` (statique) le lit sans le
  charger, en `GalaxyInfo {name, description, plugins, filePath}` (`include/Nebula/Galaxy/GalaxyInfo.h`, `name`
  = `completeBaseName()`, `plugins` = les ids de `plugins`) ; `missingPlugins(galaxy)` : ceux de ses plugins dont
  `plugins()` ne connaît pas l'id, chargés ou non. Vide = chargeable. Contrairement à `load()`, le `file` d'un plugin
  n'est pas regardé (issue #41 : une galaxy trouve ses plugins par id parmi ceux connus).
- Galaxy posée (module `Galaxy` : `include/Nebula/Galaxy/Galaxy.h`, `src/Galaxy/Galaxy.cpp`) : `instantiateGalaxy(fichier,
  nom)` crée une `Galaxy`, composant que Core construit lui-même (constructeur privé, `Universe` amie,
  `componentId()` = `Galaxy::Id` = `Nebula.Galaxy`, absent de `ComponentFactory`). Elle possède
  `components()` (noms uniques dans la galaxy), `settingsFiles()` (chacun son `name()`, sans préfixe),
  `project()` (le JSON lu, pour les positions de NebulaStudio), `problems()` ; `onStart()` démarre ses composants dans
  l'ordre (refus = arrêt de ceux démarrés), `onStop()` à rebours. Contenu = celui du fichier, qu'on édite à travers une
  instance (voir Galaxy éditée). Le
  chargement est commun à l'application et aux galaxies : `Scope {galaxy, directory, files, prefix, cycle}`,
  `loadScope()` (composants, galaxies par `loadGalaxy()`, liens, fichiers de réglages ; les plugins restent dans
  `load()`), `createGalaxy()` = `readGalaxyProject()` (lecture, cycle si son fichier canonique est déjà dans `files` —
  celui de l'application compris —, plugins par id puis chargés) + `fillGalaxy()` (nom choisi avant le contenu, remontée
  de `cycle` : une galaxy qui se contient, même plus bas, est refusée et vidée, `Galaxy::empty()` ; puis suivi des
  fichiers de réglages et `attach()` des stargates), `adopt()` / `freeName()`. Fichiers de réglages d'une galaxy :
  `SettingsFile` avec `m_galaxy` (`galaxy()` ; son `add()` accepte les composants de cette galaxy), nommés
  `<instance>.[<instance>.]<nom>`, `<nom>` étant son `name()` (`m_name`, celui que le fichier de la galaxy lui donne,
  `setName()` ; `setFileName()` le refuse pour un fichier de galaxy), numéroté (`settings2.json`) s'il est pris,
  renommés par `rename()` (`nameSettingsFiles()`), suivis par `settingsFilesChanged` une fois la galaxy remplie ;
  `allSettingsFiles()` (récursif) sert à `settingsFile()`, `start()` et `returnToEdition()`, pas à `settingsFiles()` ni
  à `save()`. `keepEditedValues()` descend dans les galaxies. `save()` écrit une galaxy `{name, type, galaxy,
  properties}`, chemin relatif à `projectDirectory()` (dossier canonique de `filePath()`), absolu sans fichier,
  `properties` (seulement si elle en expose) les valeurs de ses propriétés exposées par nom exposé (`savedValue()` :
  celles de l'édition pendant l'exécution), que `loadGalaxy()` lui redonne (`setExposedValues()`), et ajoute aux
  `plugins` ceux de ses composants (`addPluginFiles()`). Ses liens s'écrivent comme ceux de tout composant. `save()` et
  `saveGalaxy()` partagent `saveProject()`. `~Galaxy()` détache ses stargates puis appelle `releaseInterfaces()`, avant
  la destruction de ses composants. `filePath()` / `setFilePath()` : le fichier du projet, fixé par `open()` et par
  NebulaStudio.
- Galaxy éditée (issue #75) : une galaxy posée s'assemble à travers n'importe laquelle de ses instances.
  `instantiate()`, `instantiateGalaxy()` et `addStargate()` prennent un `Galaxy*` en dernier argument (`nullptr` =
  l'application), `addSettingsFile(Galaxy&)` en ajoute un à une galaxy ; `rename()`, `remove()` et
  `removeSettingsFile()` trouvent où est ce qu'on leur donne. `pathTo(instance)` (les galaxies de l'application jusqu'à
  celle qui la contient), `scope(path)` et `galaxyScope(galaxy)` refont son `Scope` (ses fichiers comptent pour les
  cycles : une galaxy qui contiendrait le fichier de celle où on la pose est refusée, ce qui suffit pour toutes les
  instances de ce fichier). Publics : `galaxyOf(composant)`, `components(galaxy)`, `galaxies(fichier)` (toutes les
  instances d'un fichier, à toute profondeur), `saveGalaxy(galaxy)` (le JSON de son fichier ; les valeurs exposées sont
  celles de l'instance) et `reloadGalaxy(galaxy)` : relit le fichier en place, même objet `Galaxy` : contrôles d'abord
  (`readGalaxyProject()`, rien ne change en cas d'échec), `aboutToBeReloaded()`, mémorise ses liens dans les deux sens
  et ses valeurs exposées, `empty()` (retire ses interfaces, ce qui délie l'extérieur, puis ses fichiers de réglages et
  ses composants), `fillGalaxy()` (laissée vide sur un cycle), relie et remet les valeurs sous les noms qui existent
  encore, `reloaded()`, et renvoie ce qui est perdu. Refusé pendant l'exécution. `restoreGalaxy(galaxy, json)` (issue
  #132) fait de même avec un JSON de `saveGalaxy()` à la place du fichier, ni lu ni écrit (contrôles par
  `canRestore()` : cycle et plugins) : `reload(…, keepValues)` sans remettre les valeurs exposées de l'instance, qui
  sont celles du JSON ; c'est ce qu'une étape de l'historique d'une scène de galaxy rend.
- Stargate (module `Galaxy` : `include/Nebula/Galaxy/Stargate.h`, `src/Galaxy/Stargate.cpp`) : composant que Core
  construit lui-même (`Stargate::Id` = `Nebula.Stargate`, constructeur privé `(Universe&, Galaxy*)`, `Universe` et
  `Galaxy` amies), créé par `Universe::addStargate(nom)` (`Stargate1`…, `nullptr` pendant l'exécution) ou lu d'un
  projet. Ce que sa galaxy a une fois posée : des interfaces `ExposedInterface {name, type, exposure}`, `Exposure` vu
  de la galaxy (`Provided` : la stargate la consomme, `Mandatory`, d'un composant ; `Consumed` : elle la fournit à des
  composants), suivies par `Passage {info, id, pointer}` (les callbacks connaissent l'`id`, le nom change), et des
  propriétés `ExposedProperty {name, component, object, property}` (pour une galaxy intérieure : `object` = elle,
  `property` = son nom exposé), qui partent à la destruction de leur objet (`forget()`). `exposeInterface()` /
  `renameInterface()` / `removeInterface()`, `exposeProperty()` (ce qu'un projet enregistre, `ProjectJson::
  savedProperties()`, ou ce qu'une galaxy expose ; pas deux fois dans la galaxy) / `renameProperty()` /
  `removeProperty()` ; un nom est unique dans la galaxy par sens (`galaxyComponents()`, toutes ses stargates) ; refus
  si `!isChangeable()` (en cours). Signal `changed`. `fillGalaxy()` appelle `attach()` sur chaque
  stargate une fois la galaxy liée : la galaxy `provide()` les fournies (avec le pointeur déjà reçu) et `consume()` les
  consommées (`consumeForGalaxy()`), `Mandatory` si un composant qui s'y lie l'est ; le pointeur et sa disponibilité
  passent dans les deux sens, de façon synchrone. Attachée (`m_attached`, aussi celle qu'`addStargate()` ajoute à une
  galaxy), elle reporte aussitôt sur sa galaxy ce qu'on expose, renomme ou retire (`provide()`, `consume()`
  `Optional`, `rename*Interface()`, `remove*Interface()`). `Galaxy::exposedProperties()` / `exposedProperty(nom)` les résolvent à travers les
  galaxies intérieures (recalculé à chaque appel). Chargement : `loadScope()` crée les stargates à leur place, expose
  leurs interfaces après tous les composants (`loadStargateInterfaces()`, type cherché par nom parmi les interfaces des
  composants, `interfaceType()`, puis `QMetaType::fromName()`), avant les liens, et leurs propriétés après
  (`loadStargateProperties()`, `readEntry()` partagé avec les fichiers de réglages). `save()` écrit `provided`,
  `consumed` (`{name, type}`) et `exposed` (entrée de réglages + `name`), chacun s'il n'est pas vide. Dans l'application
  elle-même, une stargate ne fait que passer ce que ses composants lui donnent.
- Les chemins des plugins sont relatifs à `pluginRoot` (dossier de l'exécutable par défaut, `setPluginRoot()`) ; un
  chemin absolu dans un fichier reste lu tel quel. Seuls les plugins des composants présents sont écrits. À
  l'ouverture, un plugin est d'abord cherché par son id parmi ceux connus, son fichier n'est qu'un recours.
- Ligne de commande : `addCommandLineOptions(parser)` (statique) ajoute `--plugin-dir` (répétable) et `--plugin-root`,
  `applyCommandLine(parser)` scanne chaque dossier après `plugins/`, dans l'ordre (un id déjà connu est sauté par
  `PluginManager::scan()`, par `scanPluginDirectory()`, public, qui saute avec un avertissement ce qui n'est pas un
  dossier), puis fixe la racine. `NebulaApplication` et `ExampleUniverseApp` s'en servent, avant toute ouverture de
  projet, et `StudioComponent` sur `QCoreApplication::arguments()`, ses autres options laissées de côté.
- Les liens ne sont pas mémorisés par l'application : `save()` les lit avec `Component::bindings(nom)`, une entrée de
  `bindings` par liaison d'une liste, dans son ordre, que la relecture refait en liant dans cet ordre (format inchangé).
- Exécution : `start()` mémorise les valeurs d'édition (`keepEditedValues()` : pour chaque composant et chacun de ses
  `propertyObjects()`, un `EditedValues {QPointer, propriétés, JSON}` de ce qu'écrit un projet), démarre les fichiers
  de réglages, puis les composants dans l'ordre d'instanciation ; au premier refus il arrête ce qu'il a démarré et
  revient à l'édition. Rappelé pendant l'exécution, il ne fait rien. `stop()` arrête dans l'ordre inverse, puis
  `returnToEdition()` arrête les fichiers de réglages et réécrit toutes les valeurs d'édition (`writeProperty()`).
  Pendant l'exécution, `save()` écrit ces valeurs-là (`savedValues()`), et `instantiate()` / `rename()` / `remove()`
  refusent ; les liens et les fichiers de réglages ne sont verrouillés que par NebulaStudio. `isRunning()` /
  `runningChanged`. `clear()`, `load()` et le destructeur arrêtent d'abord.
- Déchargement : `componentsOf(pluginId)` liste les instances d'un plugin, et les galaxies qui en contiennent une (même
  plus bas) ; `unloadPlugin(pluginId)` les supprime puis
  appelle `PluginManager::unload()`, et refuse (`errorString()`) pendant l'exécution.
- Fichiers de réglages (module `Settings` : `include/Nebula/Settings/SettingsFile.h`, `src/Settings/`) : ce que
  l'utilisateur de l'application change et retrouve au lancement suivant. `addSettingsFile()` / `removeSettingsFile()`
  / `settingsFiles()`, possédés par l'application (constructeur privé), écrits dans le projet (`settings`, seulement
  s'il y en a). Un `SettingsFile` a `fileName`, unique dans l'application sans tenir compte de la casse ni du dossier
  (`settingsFile(nom)`, `setFileName()` renvoie `false` sur un nom pris ; `addSettingsFile()` donne `settings.json`,
  puis `settings2.json`…, et `load()` signale un doublon, qui garde ce nom-là), `directory` (vide = `AppConfigLocation`
  de l'exécutable qui tourne, relatif = depuis son dossier)
  et des `SettingsEntry {component, object, property}`, `object` étant le composant ou un de ses `propertyObjects()` ;
  une propriété n'est que dans un fichier (`Universe::settingsFile(entry)`), seulement celles qu'un projet
  enregistre (`canHold()`), et une entrée part à la destruction de son objet (`destroyed`). `start()` (à la transition
  vers « en cours » seulement) lit chaque fichier avant de démarrer le premier composant, l'écrit avec les valeurs du projet
  s'il manque ou est incomplet, met de côté en `.bak` un fichier illisible, puis suit les NOTIFY (écriture regroupée
  par `QTimer::singleShot(0)`) ; `stop()` écrit une dernière fois et remet les valeurs du projet, mémorisées au
  `start()`. Une valeur donnée ou remise peut détruire l'objet d'une autre entrée (un `count`) : `giveValues()` parcourt
  une copie et saute ce qui est parti (`startEntry()`), `giveBackDefaults()` sort chaque entrée avant de la remettre. Le contenu inconnu d'un fichier est conservé.
  Issue #133 : `readValues(why)` lit sur le disque ce que le fichier tient pour chaque entrée (`valuesIn()`), dans leur
  ordre, du type de sa propriété (`propertyValue()`, une conversion, sans rien écrire) ; invalide pour une entrée sans
  valeur, ou dont la valeur ne convient pas ; `nullopt`, avec `why`, pour un fichier illisible, laissé tel quel
  (`readExisting()`, que `readSettings()` complète en le mettant de côté). `writeValue(entry, valeur)` écrit une valeur
  pour une de ses entrées (`writeJsonValue()`), convertie au type de sa propriété (`jsonValue()`), le reste du fichier
  gardé, le fichier créé s'il manque ; refusé (`errorString()`) pendant l'exécution, pour ce qui n'est pas une entrée,
  une valeur qui ne convient pas, et un fichier illisible ou qui ne s'écrit pas. Problèmes : `SettingsFile::failed` →
  `Universe::settingsFileFailed`, journal `nebula.settings`. `src/Application/ProjectJson.*` (privé,
  inclus depuis `src/Settings/` par `"Application/ProjectJson.h"`) : l'écriture JSON des propriétés et la clé d'un
  objet (nom, puis rang parmi ceux de ce nom), partagées avec `Universe`. `toJson()` passe par les conversions de
  `QVariant` (issue #135 : une énumération par le nom de sa valeur, des drapeaux par les leurs joints par `|`, une
  `QColor` par son nom, que QtGui sait convertir, une `QUrl` par son texte), sauf des drapeaux sans valeur ni nom pour
  0, qu'il écrit `""` (`jsonOf()`) ; `fromJson(json, type)` relit `""` comme 0 pour une énumération ou des drapeaux,
  et `writeJson()` écrit une propriété par lui : toute valeur relue d'un projet, d'une galaxy ou d'un fichier de
  réglages passe par l'un ou l'autre, jamais par `QJsonValue::toVariant()` seul. Le destructeur d'un `SettingsFile` l'arrête
  s'il tourne encore (retiré pendant l'exécution).
- Messages d'erreur de Core en anglais, non traduits (`QStringLiteral`), comme ceux de `PluginManager`.
- Pièces privées partagées (`src/`, non installées, incluses par `"Component/Components.h"`…) :
  `Component/Components.h` (`ComponentSpan`, les composants d'une application ou d'une galaxy, `componentsOfType<T>()`,
  `isAmong()`, `startInOrder()` qui arrête ce qu'il a démarré au premier refus, `stopInReverse()` ; pour `Universe`,
  `Galaxy`, `Stargate` et `SettingsFile`), `Component/MetaProperties.h` (`declaredProperties()`, pour
  `ComponentFactory` et `PropertyObject`), `Component/Connections.h` (`disconnectAll()`) et `Application/JsonFile.h`
  (`Nebula::JsonFile::read()` / `write()`, les fichiers JSON des projets et des réglages).

## Application

`Application/` (issue #73), bibliothèque `Nebula::Application` à part, comme Core (messages en anglais, non traduits,
catégorie `nebula.application`).

- `NebulaApplication` (`QApplication`, `Q_OBJECT`) : la `QApplication` d'un exécutable dont le contenu est une galaxy.
  Deux constructeurs, `(argc, argv, galaxy, options)` sans ligne de commande, et `(argc, argv, parser, galaxy,
  options)` : `setExecutableInfo()` (avant que `--version` lise la version), `processCommandLine()`
  (`addCommandLineOptions()` puis `parser.process()`, après les options déclarées par le `main`, qui relit ensuite le
  même parser), `readCommandLine()` (les `--galaxy-dir` en tête, depuis le dossier courant ; `--log-level`,
  `--log-file`, `--console`), puis `load()` : `startLog()` (le `MessageLog`), l'`Universe` (`m_universe`, construite
  après le journal, qui voit donc ses avertissements de scan ; toujours là, même sans galaxy), puis, si rien n'est
  refusé, `addPlugins()` (dossiers du code par `scanPluginDirectory()`, racine, puis `Universe::applyCommandLine()`) et
  `openGalaxy()` : `findGalaxy()` (chemin absolu tel quel, sinon le premier des dossiers qui l'a), `Universe::open()`
  strict. `refuse()` écrit `errorString()` et le journalise en critique ; `isLoaded()`, `galaxyFile()`, `universe()`.
  `exec()` (cache `QCoreApplication::exec()`, statique) : `LoadFailed` (1) aussitôt si rien n'est chargé,
  `StartFailed` (2) si `start()` refuse, sinon `QApplication::exec()` puis `stop()` (fichiers de réglages écrits pendant
  que l'application existe encore) et le code de sortie. Destructeur par défaut : `m_log` est déclaré avant
  `m_universe`, détruite d'abord.
- `ApplicationOptions` (en-tête seul) : `pluginDirectories`, `pluginRoot`, `galaxyDirectories` (`scene` par défaut),
  `executable` (`ExecutableInfo {name, version, organization, organizationDomain}`, chaque partie vide laissée à Qt),
  `logLevel` (`QtMsgType`, `QtDebugMsg` = rien de filtré), `console`, `logFile`, `logger` (`Logger`, un
  `std::function` de la forme d'un `QtMessageHandler`). Chemins relatifs : depuis le dossier de l'exécutable dans le
  code (`executableDirectory()`), depuis le dossier courant sur la ligne de commande (rendus absolus à la lecture).
- `MessageLog` (privé, `src/`) : filtre par `QLoggingCategory::setFilterRules()` (`*.debug=false`… sous le niveau ; noms
  des niveaux dans `Levels`, du moins grave au plus grave, `levelNamed()` sans casse), puis, s'il y a console, fichier
  ou logger, installe `handle()` par `qInstallMessageHandler()` : sous un `QMutex` (statiques `current`, `previous`),
  `qFormatLogMessage()` vers la sortie d'erreur (`console`, et sous Windows `AttachConsole(ATTACH_PARENT_PROCESS)` puis
  `CONOUT$` quand la sortie d'erreur ne va nulle part) ou vers le gestionnaire d'avant, puis le fichier (UTF-8, ajouté,
  vidé à chaque message, dossier créé) et le logger. Un message émis par le logger n'y revient pas : Qt, qui ne rappelle
  pas un gestionnaire en cours dans le même thread, l'écrit lui-même sur la sortie d'erreur. Le destructeur remet le
  gestionnaire d'avant et vide les règles.

## Plugins standard

`Plugins/` (issue #130) : les plugins livrés avec Nebula pour toute application, et `Nebula::Standard`, les en-têtes des
interfaces de leurs composants (`cmu_add_interface_library()`, `Qt6::Core` en `INTERFACE`), qu'un plugin lie pour les
consommer : un `MODULE` ne se lie pas. Une interface a pour type `QMetaType::fromType<Nebula::ILog*>()` dans chaque
binaire, le même par son nom, ce que `TestLogComponent` vérifie. Messages en anglais, non traduits, catégorie
`nebula.standard`.

- `LogLevel.h`, en-tête seul : `Nebula::LogLevel` (`Debug`, `Info`, `Warning`, `Critical`, dans l'ordre de gravité, pas
  de `Fatal`), `LogLevels`, `logLevelName()` / `logLevelNamed()` (sans casse, les noms de `--log-level`) et
  `qtMsgType()`.
- `ILog` (`log`) : `write(niveau, message, source)`, qui écrit `source: message`, `isWritten(niveau)`, et les raccourcis
  `debug()`… non virtuels. `ILogControl` (`logControl`) : `startLogging()`, `stopLogging()`, `isLogging()`,
  `setLevel()`, `level()` ; il ne signale rien, seul le composant notifie.
- Issue #132 : `Command.h`, en-tête seul : `Nebula::Command` (`label()`, `execute()`, `undo()`, `isObsolete()`, faux par
  défaut ; ni copiable ni déplaçable) et `Nebula::CompositeCommand` (`add()`, `execute()` dans l'ordre, `undo()` à
  rebours, obsolète dès qu'une l'est). `IHistory` (`history`) : `push()` (exécute puis garde, oublie les défaites),
  `undo()` / `redo()` (renvoient s'ils ont fait quelque chose, sautent et jettent les obsolètes), `clear()`,
  `isUndoActive()` / `isRedoActive()` ; il ne dit pas ce qu'il reste à défaire (hors périmètre de l'issue).
- `StandardPlugin` : sans `VERSION`, donc celle de Nebula ; id `FabienDG.Nebula.StandardPlugin`, lie `Nebula::Standard`
  en `PRIVATE`.

| Classe | Rôle |
| --- | --- |
| `LogComponent` | Catégorie Diagnostics. Fournit `log` (`m_log`) et `logControl` (`m_control`) dès sa construction ; propriétés `logging` (vrai), `level` (`QString` : `levelName()` / `setLevelName()`, un nom inconnu refusé avec un avertissement ; `level()` / `setLevel()` en `LogLevel`) et `category` (`log`, vide refusé par `canBeCategory()`). Un setter qui change une valeur la notifie, puis `LogFilter::share()` ; `onStart()` / `onStop()` = `LogFilter::join()` / `leave()`, comme son destructeur ; `follow()` (privé, `LogFilter` amie) prend les valeurs partagées sans les repartager |
| `Log` | `ILog` : `NamedCategory {name, category}`, le `QByteArray` que pointe la `QLoggingCategory`, qui doit lui survivre, refaite par `setCategory()` ; `write()` par `QMessageLogger().debug(catégorie)`… (`streamOf()`) si `isWritten()`, qui lit `QLoggingCategory::isEnabled()` : la catégorie suit le filtre du processus comme les autres |
| `LogControl` | `ILogControl` : appelle les setters du composant |
| `HistoryComponent` | Issue #132. Catégorie Edition. Fournit `history` (`m_history`) dès sa construction ; propriétés `maxSize` (100, négatif refusé avec un avertissement), `undoActive` et `redoActive` (vrais), notifiées ; `onStop()` vide l'historique. Marche dans tous les états |
| `History` | `IHistory` : `m_commands` (les plus anciennes d'abord), `m_done` (celles d'avant sont faites, `m_commands[m_done]` est la prochaine à refaire). `push()` oublie les défaites (`forgetUndone()`), ajoute, puis exécute par `run()`, qui pose `m_running` (une commande poussée pendant est exécutée sans être gardée, `executeAside()`, avec un avertissement ; `undo()` / `redo()` refusés, `isBusy()`), applique un `clear()` demandé pendant (`m_clearPending`), puis `trim()` : au-delà de `maxSize`, les plus anciennes faites partent, puis les défaites les plus loin d'être refaites. `dropObsoleteDone()` / `dropObsoleteUndone()` jettent les obsolètes rencontrées |
| `LogFilter` | Singleton du plugin (`instance()`) : le log du processus pendant l'exécution. `m_running`, les composants en cours ; `join()` (le dernier venu donne ses valeurs aux autres, `take()` / `giveOthers()`, puis `install()`), `share()` (de même pour un composant en cours piloté, puis `refilter()`), `leave()` (`uninstall()` avec le dernier). `filter()`, statique, est le filtre de catégories de Qt (`QLoggingCategory::installFilter()`) : il appelle le précédent (`m_previous` : celui de Qt, qui applique les règles, ou un autre), puis, actif, `disableUnder()` coupe sous `m_level` (tout si `!m_logging`) par `setEnabled(type, false)`, sans jamais réactiver. `refilter()` réinstalle le filtre en place, ce qui le fait passer sur chaque catégorie, et remet celui d'un autre s'il était en haut. `uninstall()` remet le précédent, ou, quand un autre filtre a été posé par-dessus (il appelle celui-ci), le remet et reste dessous, inactif (`m_installed`) ; `install()` ne se repose alors pas, sans quoi les deux s'appelleraient en boucle. Ce que lit `filter()` est en `std::atomic` : Qt l'appelle aussi à la création d'une catégorie, dans n'importe quel thread |

## NebulaStudio

`main.cpp` (issue #73) : une `NebulaApplication` de `NebulaStudio.nbp` (`scene/` à côté de l'exécutable), avec le nom
et la version de `NebulaStudio_info.h` mais pas son organisation (voir Pièges), le projet passé en argument (`[project]`,
après les options de `NebulaApplication`) donné, absolu, à la propriété `file` de l'instance `Project` (`openFirst()`),
puis `runWithSplash()` : le `SplashScreen` montré, fermé sur `read()`, pendant `exec()` ; `tellFailure()` dit dans une
`QMessageBox` pourquoi la galaxy ne tourne pas (`errorString()`). Le thème vient du `StudioComponent` de la galaxy.
La galaxy (`StudioPlugin/`, issue #103) assemble les parties de NebulaStudio (`Studio`, `EditionZone`, `Project`,
`PluginPanel`, `GalaxyPanel`, `ComponentPanel`, `PropertyEditor`, `ProblemList`) par des composants : deux colonnes
dans un splitter horizontal (issue #104), à gauche la zone d'édition au-dessus de `ProblemList` (splitter vertical),
qui prend ce que gagne la fenêtre, à droite un splitter vertical, masqué tant que ce qu'on glisse de ses listes en est
sorti (`DragAside`) : un `QTabWidget` en mode document (onglets **Components**, montré au départ, **Galaxies** et
**Plugins**) au-dessus du `PropertyEditor`, puis la vue d'ensemble de la zone d'édition (`SceneOverview`, issue #64).
L'exécutable compile toujours toutes les sources de `src/` (`cmu_add_executable`), dont il n'emploie que `SplashScreen`
et `Theme`. `SettingsItem`,
`SettingsDialog`, `SettingsTextDialog` et `SettingsValueEditor` sont dans `src/Settings/`, inclus par
`"Settings/SettingsItem.h"` ; `GalaxyModel`, `GalaxyViewer`,
`GalaxyPanel`, `StargateItem` et `StargateDialog` dans `src/Galaxy/`, inclus par `"Galaxy/GalaxyModel.h"`.

Pièces partagées : `RowsModel<Row, Key>` (`RowsModel.h`, base de `PluginModel`, `ComponentModel` et `GalaxyModel` :
lignes triées par leur clé, `setRows()` retire, insère à sa place et met à jour), `Inspector` (base de `PluginViewer`,
`ComponentViewer` et `GalaxyViewer` : une page en formulaire, ou un texte qui dit pourquoi elle est vide),
`PlaceholderStack` (cette page ou ce texte, aussi pour `ProblemList`, `PropertyEditor` et `ChoicePanel`),
`WarningLabel` (l'icône d'avertissement et un texte en `warning`), `EditDialog` (base de `SettingsDialog`,
`SettingsTextDialog` et `StargateDialog` : OK grisé tant que c'est refusé, rejetée si ce qu'elle édite disparaît ;
`accept()` appelle `apply()`, qui renvoie `false` pour que la fenêtre reste ouverte (un fichier qui ne s'écrit pas) ;
porte leur `DialogHeader`,
`header()`, avec l'icône donnée à son constructeur, qui est aussi celle de la fenêtre, et `layOut(contenu)` le place en
haut, sans marges, au-dessus du contenu), `DialogHeader` (le bandeau en haut d'une fenêtre, sur `AlternateBase` avec un
trait `Mid` dessous : icône, titre en `titleFont()` agrandi, en texte brut, et une explication en `PlaceholderText`),
`ChoicePanel` (issue #116 : les choix d'une `EditDialog`, cochés dans la première colonne d'un arbre déjà rempli ; un
titre en `titleFont()` et, à droite, combien sont cochés parmi les lignes qui peuvent l'être, sans en regrouper,
`refreshCount()` que la fenêtre appelle après `followCheck()` ; un filtre et une case qui ne montre que les lignes
cochées, `filterRows()` sur la première colonne, refait seulement quand ils changent ; l'arbre dans un
`PlaceholderStack`, ou pourquoi il ne montre rien : vide, rien de coché, rien de trouvé ; textes donnés par la fenêtre,
`Texts` et `WhyNone`), `PropertyTree.h` (les arbres
de propriétés de ces deux fenêtres et du `PropertyEditor` ; `makeGroupsCheckable()` et `followCheck()` : des groupes
cochés, partiellement ou non, d'après ce qui est coché sous eux, et qui cochent tout ce qu'ils regroupent ;
`filterRows()` : les lignes dont le texte, ou celui d'une ligne au-dessus, contient un texte, sans casse ;
`propertyPath(entrée)` : `objet.propriété`, ou la propriété seule, pour `SettingsItem` et `SettingsValueEditor`),
`PropertyTable.h` (les tables de valeurs du `PropertyEditor` et du `SettingsValueEditor` : `newPropertyTable()`, avec
un `PropertyDelegate` sur la colonne des valeurs, colonnes `PropertyNameColumn` / `PropertyValueColumn`, `hasEditor()`
(les types des éditeurs par défaut de Qt, `QColor`, `QUrl`, et les énumérations et drapeaux, `metaEnumOf()`),
`showTitle()` et `colorTitles()`, une ligne de titre sur toute la largeur, sur `AlternateBase`, remise sur
`PaletteChange`), `PropertyDelegate` (issue #135, le délégué de ces valeurs : `displayText()` donne le nom d'une
couleur, `colorName()`, rien pour une invalide, celui de `QStyledItemDelegate` sinon, qui écrit déjà le nom d'une
valeur d'énumération ou de drapeaux et le texte d'une URL ; `initStyleOption()` ajoute la pastille d'une couleur,
`swatch()`, bordée en `Mid`, la même pour l'état sélectionné, que le style teinterait ; `createEditor()` : un éditeur
de `ValueEditors.h` (`newEditor()`, dont `edited` écrit aussitôt, `writingAtOnce()` puis `commit()` par `sender()`),
aucun pour une couleur, celui de Qt sinon ; une couleur s'édite dans un `QColorDialog` (`ShowAlphaChannel`, parent la
table, `open()`) qu'ouvre `editorEvent()` sur un double-clic, F2 ou Espace, si la table a des déclencheurs, et qui
écrit dans le modèle par un `QPersistentModelIndex` ; `updateEditorGeometry()` déroule la liste des drapeaux sous la
cellule, remontée pour rester dans la table ; `eventFilter()` garde l'éditeur d'une URL tant que sa fenêtre est
ouverte), `ValueEditors.h` (issue #135 : `EnumEditor`, un `QComboBox` des noms, `FlagsEditor`, un `QListWidget` à
cocher, sans la valeur 0, et `UrlEditor`, un `QLineEdit` dont un bouton intégré, `addAction()`, ouvre un `QFileDialog`
d'un fichier existant ; chacun a sa valeur en propriété `USER` `value`, que `QStyledItemDelegate` lui donne et relit,
et un signal `edited`), `PropertyValues.h` (issue #135 : `metaEnumOf(QMetaType)`, l'énumération ou les drapeaux d'un
type, cherchés dans le `QMetaObject` qui les déclare d'après le nom du type, `QFlags<…>` compris ; `enumValue()`,
`colorName()`, `urlOf()` : un chemin absolu est un fichier local), `FileFollower` (issue #133 : `follow(chemin)` suit un fichier et son dossier, ceux qui existent,
`changed` quand il est réécrit, remplacé par `QSaveFile`, créé ou retiré, ou qu'un autre fichier du dossier change ;
les deux remis sous le `QFileSystemWatcher` à chaque changement, voir Pièges), `ItemShapes.h` (contour, crayon et
remplissage communs aux items), `DashedLinks`
(`DashedLinkItem.h`, les pointillés d'un `SettingsItem` ou d'un `StargateItem`), `SceneItems.h` (`objectsAmong<T>()`,
`itemsAmong<T>()`, `firstAmong<T>()` : les items d'un type parmi d'autres ; `onlyObjectAmong<T>()` : le seul, celui
sélectionné seul ; `componentItems(scène)` de `ComponentItem.h` en est un), `ContextMenu` (un menu dont chaque entrée porte ce qu'elle fait, exécuté une fois le menu
fermé, pour la scène ; `addMenu()` y ajoute un sous-menu, dont `add(sous-menu, …)` ajoute les entrées, et
`addGroup()` un `QActionGroup` exclusif, à lui, pour des entrées en boutons radio ; `exec()` renvoie ce qui a été
choisi comme une étape de l'historique le nomme : le texte de l'entrée après le titre de son sous-menu, sans `&` ni
`...`),
`PositionJson.h` (`writePosition()` / `readPosition()` : le `x` / `y` d'un projet, pour la scène et `LinkAnchors`),
`CopiedInstances.h` (issue #136, fonctions libres : ce que copie la scène dans le presse-papiers, `MimeType`
`application/x-nebula-instances`, le JSON compact de `Universe::saveInstances()` complété par `placeComponents()` et
`LinkAnchors::written()` ; `toClipboard()`, `isInClipboard()`, `fromClipboard()`, `mimeData()` / `read()`, `placed()`
(les composants sans les stargates), `topLeft(json, coin)` (le plus haut et le plus à gauche des coins des composants,
`ComponentItem::corner()`, et des ancres), `moved()` (composants et ancres décalés) et `renamed()` (les liens renommés
d'après les noms donnés)),
`PanelWidgets.h` (ce dont les parties sont faites : listes `createView()` / `createDraggableView()`, aux colonnes à la
taille de leur contenu, la dernière prenant la place qui reste, `showCurrentIn()`, colonnes, barres, boutons,
`insertPage()` et `disableWhileRunning()`), `DraggableView` (la liste de `createDraggableView()`, d'où l'on glisse
seulement : `dragStarted()` / `dragFinished()` autour de `startDrag()`, jusqu'au dépôt ou à l'abandon, chacun envoyant
aux widgets où elle est alors un événement de type `dragStartedEvent()` / `dragFinishedEvent()`, enregistré une fois),
`DragAside` (issue #104 : filtre son widget et, sur ces événements, d'une liste ajoutée avant ou après lui, masque ce
widget tant que ce qu'on glisse en est sorti, d'après les `DragMove` et `DragLeave` qu'il lit sur sa `QWindow` le temps
du glisser, un `DragLeave` par `QCursor::pos()` : fenêtre quittée ou glisser abandonné ; le remontre à la fin, sauf
s'il a quitté sa place ; rien tant qu'il n'est pas `setActive()`), `Messages.h` (`tell()` dans une barre d'état,
`MessageDuration`, `warn()`, `askToSave()`, `createPluginDirectoryLabel()`) et `ProjectFiles.h`
(`defaultProjectDirectory()`, `writeProjectFile()` par `QSaveFile`).

| Classe | Rôle |
| --- | --- |
| `Theme` (`Theme.h`) | Fonctions libres, comme `Colors.h` : `ThemeColors`, les jetons de couleur nommés par usage (`space`, `field`, `surface`, `edge`, `text`, `mutedText`, `accent`, `onAccent`…), définis une fois par schéma dans `Theme.cpp` (`themeColors(Qt::ColorScheme)`, sombre si le système ne dit rien) ; `warning` / `onWarning` (ambre, ce qui ne fonctionne pas) n'ont pas de rôle dans la palette : on les lit par `currentThemeColors()` au moment de peindre (celles du schéma montré, `shownScheme()` : le choisi, ou celui du système) ; `applyTheme(schéma)`, appelé une fois par le premier `StudioComponent`, met le style Fusion (le seul qui peint tout aux couleurs de la palette), à travers `ThemeStyle`, un `QProxyStyle` qui redessine en `Mid` (`edge`) les bords des cases à cocher (`PE_IndicatorCheckBox`, celles des listes et des menus comprises) et des champs (`PE_FrameLineEdit`), sauf quand Fusion les met en accent (focus) : Fusion les tire de la couleur des fenêtres, que les champs sombres cachaient ; charge Space Grotesk Bold depuis les ressources et donne à l'application la palette du schéma, refaite à chaque `QStyleHints::colorSchemeChanged` (`applyShownScheme()`) ; `chooseColorScheme(schéma)` (issue #118) : le schéma montré désormais, quel que soit celui du système (`chosenScheme`, variable de l'espace de noms anonyme), `Qt::ColorScheme::Unknown` pour suivre celui-ci, passé aussi à `QStyleHints::setColorScheme()` (Qt 6.8) pour les barres de titre, là où la plateforme le permet (pas `offscreen`) ; `titleFont(police)` : Space Grotesk gras, la famille de départ en repli, pour les noms des items, l'éditeur de nom, les titres de groupes du `PropertyEditor`, ceux de `DialogHeader` et de `SettingsDialog`, et le splash ; `icon(GlossaryTerm)` et `logoPath()`, voir Logo et icônes. Chaque texte et son fond, et les bords de ce qui s'utilise, tiennent le niveau AA des WCAG (4,5 et 3) |
| `SplashScreen` | `QSplashScreen` dont l'image est peinte aux couleurs de la palette depuis `NebulaStudio_info.h` (produit, version, Qt, compilateur, date de build, copyright) et la licence ; `read()` est émis une fois, deux secondes après son premier affichage ou au premier clic. `main.cpp` le montre une fois la galaxy chargée, par-dessus la fenêtre qu'elle montre en démarrant, et le ferme sur `read()` |
| `Studio` | Ce que les parties partagent : l'`Universe` éditée (`application()`, `applyCommandLine()`, qui relit plugins et galaxies), les préférences (`m_preferences`, un `QSettings` au format `preferencesFormat()`, construit avec l'organisation et le nom qu'on lui donne ; `linkStyle()` initialisé par `readLinkStyle()`, `setLinkStyle()` l'écrit et émet `linkStyleChanged`, de même `colorScheme()` / `setColorScheme()` → `colorSchemeChanged` et `isOverviewShown()` / `setOverviewShown()` → `overviewShownChanged`, voir Préférences) et ce que les parties se disent : `refreshPlugins()` → `pluginsChanged` (listes des plugins et des composants), `refreshGalaxies()` → `galaxiesChanged`, `tell()` → `told`, comme `settingsFileFailed` de l'application (la barre d'état) |
| `EditionZone` | La zone d'édition, un `QWidget` : la scène de l'application (`applicationScene()`), celles des galaxies ouvertes, la vue et sa barre au-dessus (`createBar()` : le chemin, `m_path`, puis à droite le `ComponentFinder`, `m_finder`, dont `told` passe par le `Studio`). `enterGalaxy()` crée une `EditionScene` de galaxy (`m_galaxyScenes`, l'une dans la galaxy de la précédente), `showDepth(n)` montre l'application (0) ou une galaxy (`EditionView::showScene()`, avant de fermer celles du dessous, que la vue d'ensemble quitte alors avec elle) et ferme celles du dessous, `createOverview()` (issue #64) : une `SceneOverview` de sa vue, à qui la prend, `refreshPath()` refait le chemin (`m_path`, boutons `QToolButton`, caché sur l'application, puis le bouton de **Save Galaxy**) ; retour à l'application sur `componentsChanged` quand la première galaxy ouverte n'est plus dans l'application (avant sa destruction). **Save Galaxy** (`saveGalaxyAction()`, Ctrl+Alt+S, action de la zone elle-même, que le menu **File** montre aussi, active quand une galaxy est montrée hors exécution) : `saveGalaxy()` écrit la galaxy montrée (`writeProjectFile()`) puis `readInstancesAgain()` (`Universe::galaxies()` puis `reloadGalaxy()` de chacune, celle éditée comprise, ce qu'elles perdent dans une boîte). `leaveGalaxies(depth, closing)` ferme les galaxies sous `depth`, la plus basse d'abord, en demandant Save / Discard / Cancel pour chacune que `EditionScene::isModified()` dit changée : Save écrit son fichier, ferme sa scène puis relit ses instances ; Discard ferme sa scène puis la relit ; pendant l'exécution, une galaxy changée n'est pas quittée, sauf à la fermeture (`closing`, écrite sans relecture). Le chemin, `showComponent(nom)` (retour à l'application, `EditionScene::select()` puis `centerOn()`) et `Project::saveChanges()` passent par lui. `addViewActions()` : **Find Instances** (Ctrl+Shift+F, qui donne le clavier au `ComponentFinder`, texte sélectionné, par `focusToReplace()` de `PanelWidgets.h`, comme Ctrl+F à la recherche des composants), **Zoom In** Ctrl++, **Zoom Out** Ctrl+-, **Actual Size** Ctrl+0, puis le sous-menu **Links**, un `QActionGroup` exclusif des `LinkStyles` (**Curves**, **Straight Lines**, **Right Angles**), qui appelle `Studio::setLinkStyle()` ; `drawLinks()`, sur `linkStyleChanged`, le passe à chaque scène, et `followScene()` à chaque scène créée. `addRunActions()` : **Start** valide le nom en cours d'édition de la scène montrée, puis `Universe::start()`. `selectionChanged(composant, fichier)` : le composant, ou le fichier de réglages, sélectionné seul dans la scène montrée, ou dans celle qu'on montre (`selectedComponent()`, `selectedSettingsFile()`). `refused` et `told` de ses scènes, et `galaxyPlaced` (les plugins chargés), passent par le `Studio`. Son destructeur ferme ses scènes : elle est détruite avant l'application. Historique (issue #132) : `setHistory(IHistory*)` le donne à chacune de ses scènes (`enterGalaxy()` aussi) ; `addEditActions()` : **Undo** (`QKeySequence::Undo`, Ctrl+Z) et **Redo** (Ctrl+Maj+Z d'abord, puis ceux de la plateforme, Ctrl+Y sous Windows), actions de la zone (`createEditAction()`, raccourcis valables sans menu), activées par `refreshEditActions()` (un historique, `isUndoActive()` / `isRedoActive()`, l'application ne tourne pas ; refait sur `runningChanged` et à l'ouverture du menu, l'historique ne prévenant de rien) ; `undo()` / `redo()` ne font rien sans `canStep()` : la scène montrée occupée (`isBusy()`) ou une saisie en cours (`isTyping()` : un champ a le focus, Ctrl+Maj+Z n'étant pas le Redo d'un `QLineEdit` sous Windows, ou un nom édité dans la scène). `edit(quoi, change)` passe par la scène montrée (le `PropertyEditor`) ; `clearHistory()` pour **New**, **Open...** et `remove()` (déchargement d'un plugin). Une galaxy relue ou rendue (`Galaxy::aboutToBeReloaded`) ferme les scènes plus bas que la sienne (`leaveBelow()`). Presse-papiers (issue #136) : `addEditActions()` ajoute, après un séparateur, **Cut** (`QKeySequence::Cut`), **Copy**, **Paste** et **Duplicate** (Ctrl+D), actions de la zone (`createClipboardActions()`, `describeClipboardActions()`, `createSceneAction()` : la méthode de même nom de la scène montrée, `editShownScene()`, si `canEdit()`, que `canStep()` partage : rien pendant une saisie, depuis le menu non plus), activées par `refreshClipboardActions()` (`EditionScene::canCopy()`, `CopiedInstances::isInClipboard()`, rien pendant l'exécution ; refait sur la sélection de la scène montrée, `showDepth()`, `QClipboard::dataChanged`, `runningChanged` et l'ouverture du menu) ; `instancesPlaced` d'une scène → `Studio::refreshPlugins()` |
| `Project` | Le fichier du projet de l'application, que montre la zone d'édition : `addFileActions()` (**New**, **Open...**, **Save**, **Save As...**, **Save Galaxy** de la zone, **Description...** : `QInputDialog::getMultiLineText()`, texte rogné ; ce qui remplacerait l'application désactivé pendant l'exécution), `openProjectFile()`, `saveProjectFile()`, `saveChanges()`, `openGalaxy()` (hors exécution, puis `saveChanges()` et `openProjectFile()`), `title()` et `titleChanged` (« NebulaStudio », après le nom de son fichier quand il en a un). Le fichier est donné à `Universe::setFilePath()` par `setProjectFile()`, avant `applicationScene().load()` à l'ouverture, et avant `save()` à l'enregistrement (remis en cas d'échec). Ses dialogues et ses boîtes s'ouvrent sur la fenêtre de la zone. **Description...** est une étape de la scène de l'application (`EditionScene::edit()`) ; **New** et l'ouverture d'un projet vident l'historique (`EditionZone::clearHistory()`) |
| `PluginPanel` | Les plugins (**Plugins**) : **Open plugin...** (`openPlugin()`, `m_openDirectory`), liste et `PluginViewer` ; `load()`, `unload()` (confirmation quand des instances du plugin sont dans l'application, `removing(liste)` pour ce qui les montre, puis `Universe::unloadPlugin()`) ; relu sur `pluginsChanged`, **Unload** désactivé pendant l'exécution |
| `GalaxyPanel` (`Galaxy/`) | Les galaxies d'un dossier (**Galaxies**, icône Galaxy) : dossier en lecture seule et **Choose directory...** (`directoryChosen`), liste, `GalaxyViewer` ; `setDirectory()` (vide = `scene/` à côté de l'application, relatif = depuis son dossier ; non mémorisé par NebulaStudio) ; `openRequested` pour une galaxy chargeable, par l'activation d'une ligne ou **Open** (désactivé pendant l'exécution) ; relu sur `galaxiesChanged` |
| `ComponentPanel` | Les composants (**Components**) : barre de recherche, liste à travers un `QSortFilterProxyModel` et `ComponentViewer` (voir `ComponentModel`) ; Ctrl+F (`QShortcut` du panneau, `focusToReplace()`) ; relu sur `pluginsChanged` |
| `ProblemList` | `QTreeWidget` (Component, Problem, How to fix it) d'une ligne par interface de `Component::unboundMandatoryInterfaces()`, l'aide nommant les interfaces fournies du même type (`Ticker1.display`), ou une phrase quand il n'y en a pas. Suit `Universe::componentsChanged`, et `interfacesChanged` / `objectNameChanged` de chaque composant ; reconstruit en différé (`scheduleRefresh()`, file d'événements), car ces signaux arrivent pendant une destruction. Activation d'une ligne = `componentActivated(nom)` |
| `PluginModel` / `PluginViewer` | Liste des plugins (Name, Version, State ; `IdRole`) et fiche avec boutons **Load** et **Unload** (signaux `loadRequested`, `unloadRequested`) ; `setUnloadAllowed(false)` désactive **Unload** pendant l'exécution |
| `GalaxyModel` / `GalaxyViewer` | Liste des galaxies d'un dossier (`setDirectory()`, `*.nbp` triés par nom sans casse ; Name avec l'icône Galaxy et la description en infobulle, State : Loadable, Missing plugins ou Unreadable, avec l'icône d'avertissement du style et ce qui manque en infobulle ; `FilePathRole`), une `ListedGalaxy {filePath, info, error, missingPlugins}` (`GalaxyModel::Galaxy`) par ligne, `isLoadable()` ; une non chargeable est en `mutedText` (`ForegroundRole`). `refresh()` relit tout : retire, insère à sa place, met à jour (`dataChanged`), comme `PluginModel`. Une galaxy chargeable se glisse (`ItemIsDragEnabled`, `MimeType` `application/x-nebula-galaxy`, le chemin en UTF-8), les autres non. Un `QFileSystemWatcher` suit le dossier une fois qu'il existe et chacun de ses fichiers (voir Pièges). La fiche (inspecteur) : nom, description, fichier, plugins, état, puis, si elle n'est pas chargeable, une erreur en `warning` (remis sur `PaletteChange`) avec l'icône d'avertissement : les plugins manquants et comment les faire connaître, ou pourquoi le fichier est illisible ; bouton **Open** (`openRequested`), désactivé par `setOpenAllowed(false)` pendant l'exécution. Elle suit `dataChanged`, `rowsInserted` et `rowsRemoved` du modèle |
| `ComponentModel` / `ComponentViewer` | Liste des composants (Name, Category, Plugin ; la description en infobulle), glissables au format `application/x-nebula-component` (id en UTF-8), et fiche avec toutes les propriétés ; `refresh()` insère les composants enregistrés depuis et retire ceux d'un plugin déchargé. La vue la montre à travers un `QSortFilterProxyModel` créé par `ComponentPanel` : barre de recherche (Ctrl+F), sans casse, sur la colonne et le rôle de la liste déroulante à côté (`SearchFields` dans `ComponentPanel.cpp`) : Name en `DisplayRole` (nom), Name en `ToolTipRole` (description), Category en `DisplayRole` (catégorie). Un rôle d'index de la vue se lit donc à travers le proxy (`data()`), pas de `row()` à passer au modèle |
| `EditionScene` | `QGraphicsScene` 4000×3000, grille de 20 : dépôt = `Universe::instantiate()`, tracé des liens entre points d'interface (`Component::bindInterface()`), menu contextuel des zones, des fichiers de réglages (**Add settings file** ouvre aussitôt sa fenêtre ; sur l'un d'eux : éditer, **View and edit file...** (`editText()`, une `SettingsTextDialog`), **Write default values**, supprimer) et du composant sous le curseur (renommer, couleur, supprimer), double-clic sur un fichier de réglages = `SettingsDialog` (ouvert par `open()`, pas `exec()`), touche Suppr = suppression des composants, fichiers et liens sélectionnés sauf pendant l'édition d'un nom (`remove()` détruit l'item, donc ses liens, avant `Universe::remove()` / `removeSettingsFile()` ; les liens d'abord, par `unlink()`), **Remove link** dans le menu d'un lien (`linkAt()`, l'item le plus haut), sous-menu **Bind to** dans celui du point d'une interface consommée (issue #82, `addBindEntries()`) : une entrée `Composant.interface` par interface fournie montrée qui peut s'y lier (`linkTargets()` : `canLinkTo()`, dans l'ordre des composants puis de leurs interfaces), cochée quand un `LinkItem` les relie, en boutons radio pour une interface seule (`QActionGroup` `ExclusiveOptional`), en cases à cocher pour une liste ; la choisir appelle `toggleLink()` (`unlink()` si elle est cochée, `link()` sinon) ; une entrée désactivée dit quand aucune ne le peut, bord et ligne du point d'une interface (issue #117 : Maj + clic droit sur le point, `showDotMenu()` depuis `contextMenuEvent()`, à la souris seulement, `shiftClickedDot()`, Maj+F10 ouvrant le menu au clavier, ouvre à la place son propre menu : **Dot on the left edge** et **Dot on the right edge** en boutons radio, `addSideEntries()` dans un `ContextMenu::addGroup()`, le bord où il est coché, qui appellent `ComponentItem::setInterfaceSide()`, puis le sous-menu **Swap with**, `addSwapEntries()` : une entrée par autre interface du composant, de haut en bas, `nom (consumed)` ou `nom (provided)`, qui appelle `ComponentItem::swapInterfaces()`, ou une entrée désactivée quand il n'en a pas d'autre), ancres des liens (issue #106 : double-clic sur un lien = `LinkItem::addAnchor()`, sur une ancre = `removeAnchor()`, par `doubleClick()` après `editSettingsAt()` ; **Add anchor point** et **Remove anchor points** dans le menu d'un lien, **Remove anchor point** dans celui d'une ancre, `anchorAt()` ; `drawLink()` ajoute à la scène un lien refait, le rend éditable ou non, puis lui redonne les ancres de `m_anchors`, un `LinkAnchors`, qui reprend les points des liens de la même interface fournie déjà tracés) ; points de dérivation (issue #126 : Maj + glisser depuis un lien ou une ancre, `startLinkAt()` puis `startBranch()`, `m_branch` : `Branch {link, anchor, position}`, `m_linkSource` étant l'interface fournie du lien ; le tracé part de `pendingStart()`, `LinkItem::branchStop()` ; au relâchement sur une interface consommée qui peut s'y lier, `linkThrough()` la lie par `link()` si elle ne l'est pas déjà, puis fait passer son lien par `anchorsUpTo(junctionOf())`, le point ajouté au lien s'il n'en est pas un, sauf s'il y passe déjà, `goesThrough()` ; Échap abandonne le tracé, `abandonLink()` ; double-clic sur un point de dérivation et **Remove junction point** dans son menu, `addRemoveEntry()` = `LinkItem::removeJunction()` ; le menu d'un lien agit sur ceux qui partagent son trait là où il est ouvert, `linksAlong()`, **Remove link** devenant **Remove links**), `unlink(liens)` lit ce qu'ils lient avant de délier chacun de son fournisseur (`Component::unbindInterface(nom, fournisseur, nomFourni)` : une liste garde ses autres liens), puis `refreshLinks()` les retire, `selectedComponent()`, `selectedSettingsFile()` (issue #133, `onlyObjectAmong()`), `select(composant)` (le sélectionne seul, renvoie son item) ; `remove(liste)` public pour le déchargement d'un plugin ; `save()` / `load()` / `reset()` pour les projets ; signaux `refused(why)` quand un nom est refusé ou un fichier non écrit, `told(what)` quand il l'est, passés par la zone d'édition au `Studio`, que la barre d'état de la fenêtre affiche. Verrouillée pendant l'exécution (`isEditable()`, suit `runningChanged`) : `setEditable()` passe à chaque item (`ComponentItem::setEditable()`, `ItemIsMovable` d'un `SettingsItem`, `ZoneItem::setEditable()`), pas de dépôt ni de tracé de lien, double-clic sur un fichier de réglages ignoré, menu contextuel et Suppr remplacés par `refuseEdition()` ; la sélection reste. Galaxies : dépôt d'un `GalaxyModel::MimeType` = `Universe::instantiateGalaxy()` (refus et problèmes dans la barre d'état, `galaxyPlaced` pour rafraîchir les modèles), item `setIcon()` Galaxy et `setOpenable()`, **Open galaxy** dans son menu ; son `openRequested` et ce menu émettent `galaxyOpened(Galaxy&)`. Stargates : **Add stargate** (zone vide, `Universe::addStargate()`, `StargateDialog` aussitôt), **Expose...** sur le point d'interface d'un composant qui n'est pas une stargate (`expose()` : nouvelle stargate à `ExposedDistance` du côté du bord où est le point de l'interface, `exposedPosition()`, son propre point sur le bord opposé, tourné vers lui, `showExposing()`, nom de l'interface ou suivi d'un numéro, liée, puis `editInterfaceName()` aussitôt), **Edit stargate...** dans son menu et son `openRequested` (`editStargate()`, refusé hors édition), `interfaceNameEdited` → `renameExposed()` (`Stargate::renameInterface()`, refus dans la barre d'état). Les liens ne sont plus tracés à la main : `refreshLinks()` fait correspondre les `LinkItem` aux `bindings()` des composants montrés (un par liaison d'une liste) (garde ceux qui y sont, détruit les autres, ajoute ce qui manque), appelé par `show()`, `link()`, `expose()`, le `accepted` d'une `StargateDialog` et `ComponentItem::interfacesRebuilt` ; `show()` refait aussi les pointillés des `StargateItem` une fois tous les composants montrés. `setLinkStyle()` / `linkStyle()` (`m_linkStyle`, `Curved` au départ) : le style de chaque `LinkItem`, de ceux déjà montrés comme de ceux créés ensuite, et du lien en cours de tracé ; rien n'en est écrit par `save()`. Second constructeur `EditionScene(application, factory, galaxy)` : montre une galaxy (`galaxy()`) par `show()`, commun avec `load()`, aux positions de `Galaxy::project()`, et l'édite comme l'application : chaque appel à l'application passe `m_galaxy` (`instantiate()`, `instantiateGalaxy()`, `addStargate()`, `addSettingsFile(Galaxy&)`), `save()` = `Universe::saveGalaxy()` plus les positions, `isModified()` compare `save()` à `m_saved` (pris quand elle est montrée) ; sur `Galaxy::aboutToBeReloaded` elle retire ses items (en gardant les noms sélectionnés), sur `reloaded` elle remontre la galaxy, resélectionne et reprend `m_saved` ; ses galaxies s'ouvrent de même. Historique (issue #132) : chaque édition est une étape (`SceneCommand`) de l'historique que lui donne `setHistory()` : un geste (`Gesture`, de l'appui du bouton gauche, ou du double-clic, à son relâchement : `beginGesture()`, `nameGesture()` par `dragName()`, `finishLink()`, `removeAnchorAt()`, `anchorLinkAt()`, puis `endGesture()`), une entrée de menu (`execMenu()`, le nom de l'entrée suivi du composant sous le curseur), Suppr, un dépôt, un nom (`renameEdited()`, `renameExposed()`, `retitle()` par `ZoneItem::setTitleEdited()`), une fenêtre acceptée (avant pris à son ouverture : elle bloque la fenêtre), et `edit(quoi, change)` pour ce qui vient d'ailleurs. `step()` prend un `snapshot()` avant, `record()` compare après et pousse s'il diffère ; une étape finie pendant une autre (`m_pending`, `m_gesture`) les fait partir d'après elle (`startAfter()`). `snapshot()` = `save()`, les fichiers des galaxies de l'application entiers (`setFilePath({})` le temps de l'écrire : le projet peut être enregistré ailleurs entre-temps). `restore(état, quoi)` : refusé pendant l'exécution, rien si l'état est déjà montré ; `contentOf()` (le JSON sans `x`, `y`, `color`, `sides`, `rows`, `anchors`, `zones`) égal : items refaits (`restoreItems()` : `removeItems()` puis `show()`), composants intacts ; seule la description diffère (application) : `setDescription()` ; sinon `load()` (application) ou `Universe::restoreGalaxy()` (galaxy, `m_saved` gardé) ; garde les noms sélectionnés, annule le geste en cours, dit `quoi` par `told`. Copier-coller (issue #136) : `copiedItems()` (les `ComponentItem` sélectionnés, sans les `StargateItem`), `canCopy()`, `copied()` (`Universe::saveInstances()` puis `placeComponents()` et `m_anchors.written()`, comme `save()`), `copy()` (presse-papiers, `told` : `Source1 copied`, `2 instances copied`), `cut()` (copie puis `removeSelected()`, une étape `Cut …` : toute la sélection, comme Suppr), `paste()` et `pasteAt(position)` (`canPaste()`, puis `pasteMoved()` : une étape `Paste …` nommée d'après ce qui est collé, sélectionné), `duplicate()` (`copied()` collé sans passer par le presse-papiers, étape `Duplicate …`), tous refusés hors édition (`checkEditable()`). Décalage : `pasteOffset()` compte dans `m_pasted` (`Pasted {instances, count}`) les collages successifs du même JSON, `PasteStep` (deux pas de grille) de plus chacun ; `pasteAt()` met le coin en haut à gauche (`CopiedInstances::topLeft()` avec `ComponentItem::corner()`) là où le menu est ouvert. `insert()` : `Universe::insertInstances()` dans `m_galaxy` (refus `Nothing is pasted: …` par `refused`, problèmes par `told`, `tellUnrestored()`), `showInserted()` (`showSaved()` de chacune avec son JSON, dans l'ordre, sélectionnée ; les liens renommés, `CopiedInstances::renamed()`, lus par `m_anchors.read()` puis `refreshLinks()`, ce qui refait aussi les points de dérivation), puis `instancesPlaced`. Menu : `addCopyEntries()` (**Cut**, **Copy**, **Duplicate** sur une instance, `addCopyEntry()` puis `actOn()` : celle-ci seule, sélectionnée d'abord, si elle n'est pas sélectionnée ; désactivées pour une stargate) et **Paste** dans `addSceneEntries()` (`addPasteEntry()`), désactivé sans instances copiées |
| `SceneCommand` | Issue #132. Une étape de l'historique d'une `EditionScene` : `QPointer` de la scène, ses snapshots avant et après ; `undo()` / `execute()` = `restore()` avant / après, avec `Undone: …` / `Redone: …` ; exécutée en étant poussée, elle ne change rien (la scène montre déjà l'après). Obsolète quand la scène est détruite (galaxy quittée) |
| `EditionView` | `QGraphicsView` de la scène, avec un zoom (`zoom()`, `setZoom()` borné entre 0,25 et 4, `zoomIn()` / `zoomOut()` par pas de 1,25, `resetZoom()`, signal `zoomChanged`) : touches + et - autour du centre, si la scène ne les a pas prises (un nom en cours d'édition les reçoit d'abord), Ctrl+molette autour du point sous le curseur (défilement recalculé à la main, pas d'ancre `AnchorUnderMouse`). Bouton du milieu seul maintenu : déplacement de la vue (`startPan()`, `endPanIfReleased()`, curseur main fermée), sans que la scène voie ces clics (sélection et nom en cours d'édition restent) ; vue non interactive (`setInteractive(false)`) le temps du déplacement, sans quoi chaque défilement rejoue à la scène le dernier événement souris, là où était le curseur avant ; fin au premier événement sans le bouton du milieu, son relâchement se perdant quand une autre fenêtre prend la souris. Ni le zoom ni le déplacement ne sont écrits dans le projet. Quand le viewport change de taille en bougeant dans sa fenêtre (un panneau à côté masqué ou remontré, issue #104), `keepInPlace()` fait défiler d'autant qu'il a bougé depuis le dernier redimensionnement (`m_origin`) : la scène reste en place à l'écran, le panneau la découvre ou la recouvre ; puis `keepSelectionShown()` garde visible l'item sélectionné seul, s'il l'était entier avant (celui qu'on vient de déposer là où le panneau revient). Pour ce qui la suit (issue #64) : `showScene()` à la place de `setScene()`, qui émet ensuite `sceneChanged`, et `shownArea()`, ce qu'elle montre de la scène, en coordonnées de la scène, dont `shownAreaChanged` est émis par `scrollContentsBy()`, `resizeEvent()`, `setZoom()` et `showScene()` |
| `SceneOverview` | Issue #64. `QGraphicsView` de la scène d'une `EditionView` (`m_view`, détruite après elle) : toute la scène (`fitInView()` `KeepAspectRatio` à chaque redimensionnement, `showScene()` et `sceneRectChanged`), sans barres de défilement ; `drawBackground()` ne fait pas celui de la scène (ni grille, ni texte d'une scène vide) : le fond en `Window` et l'aire de la scène en `Base`, bordée en `Mid` ; `drawForeground()` encadre `shownArea()` de la vue en `Highlight` (crayon cosmétique de 2, rempli à 15 %), redessiné sur `shownAreaChanged`. Suit `sceneChanged` de la vue. Non interactive (`setInteractive(false)`), `Qt::NoFocus`, sans dépôt : clic gauche ou glisser = `m_view.centerOn()` du point sous le curseur ; les autres boutons ne font rien. `sizeHint()` 240×180 |
| `ComponentFinder` | `QLineEdit` au-dessus de la vue (issue #63) : trouve les instances de la scène que montre la vue (`QGraphicsView::scene()` au moment de chercher : l'application ou la galaxy ouverte, pas l'intérieur des galaxies posées), par leur nom ou le nom de leur composant (`ComponentItem::componentName()`), sans casse, triées par nom (`QCollator` numérique : `Greeter2` avant `Greeter10`). `QCompleter` (`MatchContains`) sur un `QStandardItemModel` refait à chaque `textEdited`, que `QLineEdit` émet avant de faire compléter : une ligne par instance, son nom (icône de son composant, Astre par défaut) puis le nom de son composant, cherchés dans `SearchRole` (les deux sur deux lignes : pas à cheval), `NameCompleter::pathFromIndex()` complétant par le nom ; popup `QTreeView`, modes de colonnes globaux (par colonne, `QHeaderView` affirme en Debug tant qu'elle n'existe pas). `find(texte)` (Entrée, ou une proposition choisie) : celle nommée ainsi seule s'il y en a une, sinon la suivante de la sélection parmi celles trouvées (la première après la dernière), sélectionnée seule et centrée (`centerOn()`) ; le champ garde le clavier ; `told` dit laquelle (`2 of 3`) ou que rien n'est trouvé, la sélection restant alors. Fonctionne aussi pendant l'exécution |
| `ComponentItem` | `QGraphicsObject` sélectionnable, déplaçable tant qu'il est éditable (`setEditable()`) ; `corner()` (statique, issue #136) : son coin en haut à gauche depuis sa position, quelles que soient ses interfaces (`Header`) ; sur `interfacesChanged`, met à jour ses `InterfaceItem`, ou les refait tous, sans leurs liens, quand une interface montrée a été renommée ou retirée (celles d'une stargate), et émet alors `interfacesRebuilt` ; `interfaceItem(genre, nom)` ; `setInterfaceSide(genre, nom, côté)` (issue #117) : le bord du point d'une interface, gardé dans `m_sides` par `InterfaceItem::Key` (genre et nom) quand ce n'est pas celui de son genre (`InterfaceItem::defaultSide()`) ; `interfaceRows()` : les interfaces de haut en bas, celles de `m_order` (rempli par `swapInterfaces(clé, clé)` ou `setInterfaceOrder(clés)`, vidé quand l'ordre redevient celui des genres) puis les autres, consommées puis fournies, une interface déclarée après un échange venant donc en dernier. Bords et lignes survivent aux `InterfaceItem` refaits et au renommage sur place (`nameEditedInterface()`, `keepPlace()` : gardés aussi sous le nouveau nom avant de le demander, sauf s'il est pris), et partent avec l'interface (`forgetLost()`, rien pour un nom inconnu) ; `placeInterfaces()` les applique (une ligne par interface dans l'ordre d'`interfaceRows()`, `place()` : `setSide()` avant `setPos()`, pour que les liens suivent dans le bon sens) ; `setRows()` : lignes nom / emplacement sous les interfaces (propriétés d'une stargate) ; `setInterfacesRenamable()` + `editInterfaceName()` : nom d'une interface édité sur place par double-clic, `NameEditor` enfant du `ComponentItem` (pas de l'`InterfaceItem`, que le renommage détruit), signal `interfaceNameEdited(kind, nom, nouveau)`, `isNameEdited(interface)` pour que l'`InterfaceItem` ne peigne pas son nom pendant ce temps ; marque d'état dans le coin, redessinée sur `stateChanged` (rien = `Editing`, triangle = `Running`) ; dans le coin opposé, « ! » dans un disque `warning` tant que `unboundMandatoryInterfaces()` n'est pas vide (`m_unbound`, relu par `refreshInterfaces()`, avec l'infobulle et `InterfaceItem::setMissing()`) ; couleur optionnelle (`color()` invalide = couleurs du style), opacité comprise, peinte derrière ses noms seulement ; `editName()` édite le nom sur place (double-clic sur les noms, s'il est éditable) et émet `nameEdited`, que la scène applique par `Universe::rename()` ; `setIcon()` : icône devant le nom de son composant (`componentIcon()`, nulle par défaut), dont le nom est `componentName()` ; `setOpenable(à propos, comment l'ouvrir)` pour une galaxy ou une stargate : infobulle (à propos à la place de l'id, puis comment l'ouvrir), double-clic sous la ligne du nom (partout s'il n'est pas éditable), hors d'un nom d'interface renommable, = `openRequested()`, après `ungrabMouse()` : ce qui s'ouvre (fenêtre, autre scène) prend le relâchement, et la scène garderait l'item saisi, sans menu contextuel ni Suppr |
| `StargateItem` (`Galaxy/`) | `ComponentItem` d'une `Nebula::Stargate` : icône Stargate devant « Stargate », interfaces renommables, une ligne par propriété exposée (nom exposé, `Composant.[objet.]propriété`, refaites sur `Stargate::changed` et le renommage de leurs composants), infobulle de ce qu'a la galaxy (fournit, consomme, expose), `DashedLinkItem` vers les composants de ces propriétés (`refreshLinks()`, public pour `EditionScene::show()`), suivis par `xChanged` / `yChanged` ; double-clic = `openRequested` |
| `StargateDialog` (`Galaxy/`) | Issue #116, à l'image de `SettingsDialog`. Un `DialogHeader` (icône Stargate, son nom, à quoi elle sert et, dans une galaxy, le nom de celle-ci, `about()`), puis `newContent()` : un `ChoicePanel` (**Interfaces and properties**, `N of M exposed`, filtre et **Exposed only**), le problème et les boutons. Arbre composant (ceux d'où est la stargate, `components()` : l'application ou sa galaxy ; sauf stargates ; `topRow()` : icône Astre, ou Galaxy pour une galaxy posée, `titleFont()`) → interfaces consommées, fournies, puis propriétés (celles que `SettingsFile::canHold()` accepte, groupes d'objets comme `SettingsDialog`, ou les propriétés exposées d'une galaxy), colonnes Type, Exposed as (un double-clic sur une ligne la coche puis édite ce nom, `editItem()`), Exposed by (une autre stargate : ligne désactivée avec l'icône Stargate, `disableForOther()`, sauf pour une liste, que `boundTo()` lit parmi ses `bindings()` et qui garde ses autres liaisons quand on l'expose) ; dernier groupe « Bound to none of these instances » (icône Stargate) : ce que la stargate expose sans ligne (`Kind::Kept`). Composants et groupes cochables (`makeGroupsCheckable()`) : `edited()` sur `itemChanged`, en colonne des cases `checkChanged()` = `followCheck()`, puis `tidyNames()` sur la ligne et celles sous elle dans l'ordre de l'arbre (un nom par défaut à chaque cochée qui n'en a pas, que les précédentes n'ont pas ; plus de nom pour une décochée), puis `refreshCount()`. Cocher donne un nom par défaut (`defaultName()`, unique sauf deux consommées de même type, qui partagent : `canShare()`), `check()` refuse un nom vide ou en double (OK grisé, raison en `warning`, remise sur `PaletteChange`). `apply()` au `accepted` : propriétés retirées puis ajoutées, consommateurs déliés, interfaces retirées puis exposées dans l'ordre de l'arbre, puis liées |
| `NameEditor` | `QGraphicsTextItem` enfant d'un item, pour éditer son nom sur place (titre d'une zone, nom d'un composant ou d'une interface d'une stargate, aligné à gauche, au centre ou à droite) : Entrée ou un clic ailleurs valide, Échap abandonne. `Colors.h` : `over()`, une couleur translucide vue sur un fond, `luminance()` et `contrast()` au sens des WCAG, et `textColorOver()`, noir ou blanc, celui qui contraste le plus avec le fond (4,58 au moins) |
| `InterfaceItem` | `key()` (`Key`, genre et nom, ce qui désigne une interface quand ses items sont refaits), point sur le bord (consommée à gauche, fournie à droite, `defaultSide()`, tant que son composant ne le met pas sur l'autre : `Side`, `side()`, `setSide()`, le nom à côté du point, `nameRect()`, dans le composant ; deux cercles décalés vers l'extérieur pour une liste, `isList()`, `ListOffset`, `outward()`), `componentItem()`, infobulle qui dit aussi Maj + clic droit, astérisque après le nom d'une consommée `Mandatory`, point et nom en `warning` quand elle n'est liée à rien (`setMissing()`), `typeName(QMetaType)` statique (sans l'`*`), `Type = UserType + 1`, `canLinkTo()`, infobulle, estompage, `nameRect()` ; connaît ses `LinkItem` (`linkTo(autre)` : celui qui la relie à une autre) |
| `LinkItem` | Trait entre deux points, Z = -1 (-0,75 une fois sélectionné : par-dessus les autres liens), `Type = UserType + 3`, se supprime avec le premier `InterfaceItem` détruit ; `provided()`, `consumed()` ; tracé dans son `style()` (`LinkStyle.h` : `Curved`, la courbe de Bézier d'avant, `Straight`, `RightAngled`, droit à mi-chemin, en revenant à mi-hauteur quand le consommateur n'est pas assez loin dans le sens où part le lien, `backAcross()`, ou autour quand les deux points regardent du même côté, `around()` ; `setStyle()` le retrace), `path()` ; `route(style, Stop, Stop)` statique sert aussi au lien en cours de tracé : un `Stop {point, leftwards}` est un point et le sens, horizontal, où le trait y passe, `stopAt(point)` celui d'un point d'interface (sortie par le bord du fournisseur où il est, entrée par celui du consommateur : vers la gauche pour un fourni à gauche ou un consommé à droite, issue #117), le curseur du lien en cours de tracé dans le sens de son départ ; une ancre est traversée dans le sens du point d'avant vers celui d'après (`stopThrough()`, vers la droite à la verticale), un point de dérivation dans le sens du point d'avant vers lui, quel que soit le lien. Sélectionnable par clic (`shape()` : le trait élargi à 10), en `Highlight` une fois sélectionné, plus épais au survol là où il est vu (`itemAt()`, voir Pièges), infobulle de ce qu'il lie, relue à chaque survol (les noms changent) ; `itemChange()` refuse la sélection tant qu'une vue trace un rectangle (`rubberBandRect()`) : un lien qui le traverse partirait avec ce qu'il sélectionne. Ancres (issue #106) : `anchors()` (positions), `anchorItems()`, `setAnchors(positions)`, `setAnchorItems()`, `addAnchor(position)` (insérée dans le tronçon le plus proche, `nearestStretch()`, distance sur 32 points de chaque `route()`, et dans chaque lien qui partage ce tronçon ; renvoie le point), `removeAnchor(ancre)` statique (de chaque lien qui y passe) ; le tracé passe par chacune, une `route()` d'un point au suivant (`m_stretches`) raccordée par `connectPath()` ; le callback du constructeur (`anchored`) est appelé à chaque changement, `anchorsChanged()`, qui retrace aussi les autres liens de son interface fournie (`followSiblings()` : leurs points de dérivation et qui dessine quoi en dépendent) ; `ends()` (`Ends {consumer, consumed, provider, provided}`, clé de `LinkAnchors`) ; `setEditable()`. Points de dérivation (issue #126) : les liens d'une même interface fournie partagent les `AnchorItem` par où ils passent, chacun par les mêmes avant eux (un arbre de traits depuis le point de l'interface) ; `setAnchors(positions)` reprend ceux d'un autre lien de l'interface fournie qui commence par les mêmes positions (`sharedAnchors()`, `sharedAfter()` : c'est ainsi qu'un projet et les liens refaits les retrouvent), `after(ancre)`, `anchorsUpTo(ancre)`, `linksAlong(position)` (ceux qui partagent le tronçon le plus proche : ceux du point où il finit, ou lui seul jusqu'à son interface consommée), `branchStop(position)` (le départ d'un lien qui en dérive, pour le tracé en cours), `removeJunction()` statique (chaque lien qui y passe ne garde que ses points d'après, et repart de son interface fournie). Un tronçon partagé n'est peint que par le premier lien du point où il finit (`ownPath()`, `drawsStretch()`) ; un lien sélectionné ou survolé peint tout son trait. Survolé ou cliqué (`mouseReleaseEvent()`, après la sélection de la classe de base) sur un tronçon partagé, ce sont tous les liens qui le partagent qui le sont (`hoverAlong()`, `selectAlong()`) |
| `AnchorItem` | Point par où passent des liens, item de premier niveau (pas enfant d'un lien), Z = -0,5 (au-dessus des liens, sous les composants), `Type = UserType + 4`, position = le point dans la scène ; `links()` (dans l'ordre d'arrivée : le premier peint le trait qu'ils partagent jusqu'à lui), `addLink()` / `removeLink()` (appelés par `LinkItem`, qui le détruit quand plus aucun lien n'y passe), son destructeur fait oublier le point aux liens qui y passent encore (`QGraphicsScene::clear()` peut le détruire avant eux) ; `isJunction()` : les liens qui y passent n'y vont pas tous au même point ensuite, peint en disque plein (losange sinon), avec son infobulle (`refresh()`, à chaque tracé de ses liens) ; se déplace lui-même dans `mouseMoveEvent()` sans `ItemIsMovable` (un item mobile entraînerait les éléments sélectionnés), puis `LinkItem::anchorsChanged()` de chacun de ses liens ; non éditable, il laisse le clic au lien dessous, qui se sélectionne |
| `LinkAnchors` | Les ancres des liens d'une scène, `std::map<LinkItem::Ends, QList<QPointF>>` : elles survivent aux `LinkItem` refaits avec les `InterfaceItem` (#86) ; `of()`, `keep()` (callback des liens), `keepOnly(liens)` (après `refreshLinks()` et la suppression d'un composant : celles d'une liaison défaite partent), `read()` / `written()` : le tableau `anchors` (`{x, y}`) de chaque entrée de `bindings`, retrouvée par les noms |
| `ZoneItem` | Zone colorée titrée, `Type = UserType + 2`, Z = -aire ; déplacement par le titre avec son contenu (composants et fichiers de réglages par leur centre, zones entières), redimensionnement par les bords, édition du titre sur place ; sa `shape()` ne couvre que titre et bords ; `setEditable(false)` lui retire boutons et survol |
| `SettingsItem` / `DashedLinkItem` | `QGraphicsObject` d'un `SettingsFile` : feuille au coin plié, nom du fichier, infobulle (chemin, propriétés tenues) ; refait ses `DashedLinkItem` (`src/`, traits pointillés de centre à centre, Z = -1, dans la scène et non enfants, détruits avec lui, qui préviennent leur item par un callback quand un autre les détruit, comme `QGraphicsScene::clear()`) sur `SettingsFile::changed` et à son ajout à la scène, et les suit par `xChanged` / `yChanged` des deux items ; `StargateItem` fait de même |
| `SettingsDialog` | Issue #105. Un `DialogHeader` (icône Comet, le nom tapé ou « Settings file », à quoi sert le fichier et, pour un fichier de galaxy, le nom de fichier de cette instance, `about()`), puis, marges comprises, `newContent()` : nom (`SettingsFile::name()`, celui que lui donne le fichier de sa galaxy pour un fichier de galaxy ; appliqué par `setName()`), dossier (bouton qui le vide, **Browse...**), chemin résultant en `PlaceholderText`, pourquoi le nom est refusé (vide, ou celui d'un autre fichier, OK alors grisé) dans un `WarningLabel` (`refreshFields()`) ; un `ChoicePanel` (`m_properties`) : titre **Properties** et `N of M held` (les propriétés cochées parmi celles qu'il peut tenir), filtre (`filterRows()` sur la colonne des noms) et **Held only**, et pourquoi rien n'est montré (`whyNoProperty()`) ; arbre composant (icône Astre, `titleFont()`) → groupe d'objet → propriétés que `SettingsFile::canHold()` accepte, à cocher, colonnes Property, Default value (la valeur du projet), Held by ; composants et groupes cochables (`makeGroupsCheckable()`, `followCheck()` sur `itemChanged`, puis `ChoicePanel::refreshCount()`) ; celles d'un autre fichier sont désactivées avec son nom et l'icône Comet ; les groupes d'objets ne sont ouverts que s'ils ont une propriété tenue, ou qu'un texte filtré trouve sous eux ; appliqué au `accepted`, seulement par les lignes de propriétés (`m_entries`) |
| `PropertyEditor` | `QTableWidget` des propriétés inscriptibles de type bool, int, uint, double, QString, QDate, QTime, QDateTime, et, issue #135, des énumérations, des drapeaux, QColor et QUrl (`hasEditor()`, éditées par le `PropertyDelegate` de `newPropertyTable()`) : celles du composant, puis celles de ses `propertyObjects()` (sans `objectName`), sans nom à la suite, avec nom sous une ligne de titre par nom ; pour une galaxy, ses `exposedProperties()` sous leur nom exposé (`Row::name`, `Row::component` = le composant intérieur, pour l'italique et l'infobulle) ; nom en italique pour une propriété tenue par un fichier de réglages, icône Stargate et infobulle pour une propriété qu'une stargate d'à côté expose (`stargates()` : celles de l'application, ou de la galaxy où est le composant, `Universe::galaxyOf()` ; `exposedBy()`) ; suit les signaux NOTIFY, `propertyObjectsChanged` (reconstruit en différé, `scheduleRebuild()` : la valeur écrite depuis la table, un `count`, ajoute ou détruit des objets pendant que la cellule éditée le signale), `settingsFilesChanged`, `componentsChanged`, `changed` de ces stargates, `reloaded` d'une galaxy montrée et `runningChanged` : en lecture seule pendant l'exécution (`NoEditTriggers`) ; une galaxy sans propriété exposée, ou une stargate, a un texte qui dit de double-cliquer dessus. Deux pages dans un `QStackedWidget` (`m_pages`, issue #133) : celle d'un composant (`m_stack`, la table dans son `PlaceholderStack`) et celle d'un fichier de réglages (`m_settings`, un `SettingsValueEditor`) ; `setComponent()` montre la première, `setSettingsFile()` la seconde (`nullptr` = rien, la première), `showSelection(composant, fichier)` le fichier s'il y en a un ; signal `refused` (celui du `SettingsValueEditor`). Une valeur écrite passe par `setEdit()` (`Edit`, une fonction `(quoi, change)`, celle de `EditionZone::edit()` dans la galaxy ; écrite directement sans), nommée par `stepName()` (`Change count of Outlets1`) |
| `SettingsValueEditor` (`Settings/`) | Issue #133. Les valeurs que tient le fichier d'un `SettingsFile` (`setFile()`), éditées dans son fichier : au-dessus, un en-tête (`newHeader()`) avec le chemin (`PlaceholderText`, sélectionnable), une note (le fichier n'existe pas encore, ou l'application tourne, `noteFor()`) et un `WarningLabel` (illisible), puis une `newPropertyTable()` : une ligne de titre par instance (`showTitle()`, dans l'ordre où elles viennent dans les entrées, `instancesOf()`), puis une ligne par entrée (`Row {index, entry, property, name}`, `name` = `propertyPath()`), sa valeur lue par `SettingsFile::readValues()` (`refresh()`) : invalide = la valeur par défaut (`property.read()`), en italique avec une infobulle (`showValue()`) ; un type sans éditeur (`hasEditor()`) n'est que montré. Une valeur éditée (`itemChanged`) va à `SettingsFile::writeValue()`, un refus à `refused`, puis `refresh()`. Éditable (`setEditable()`) tant que le fichier se lit et que l'application ne tourne pas. Refait sur `SettingsFile::changed`, `objectNameChanged` de ses instances (`m_instances`, des `QPointer` : le fichier les tient sous leur nom) ; relu sur `runningChanged` et sur `changed` de son `FileFollower`, qui suit à chaque `refresh()` le chemin du fichier (son dossier peut exister depuis) ; `setFile(nullptr)` à la destruction du fichier. Sans entrée, un texte dit de double-cliquer sur le fichier |
| `SettingsTextDialog` (`Settings/`) | Issue #133, une `EditDialog` : le texte d'un fichier de réglages, tel quel. `DialogHeader` (icône Comet, nom du fichier, à quoi il sert et son chemin, `tellAbout()`), un `QPlainTextEdit` en police fixe sans retour à la ligne (`newTextEdit()`, texte d'attente quand le fichier n'existe pas), un `WarningLabel` et les boutons. `check()` sur `textChanged` : OK grisé tant que le texte n'est pas un objet JSON (`jsonRefusal()`, avec la ligne de l'erreur, `notJson()` ; rien de tapé = pas un objet) ; sinon l'avertissement dit, s'il l'est (`m_outdated`), que le fichier a changé depuis. `apply()` écrit le texte tel quel, en UTF-8, par `QSaveFile`, dossier créé (`writeText()`), et renvoie `false` en disant pourquoi s'il ne s'écrit pas. Suit le fichier par un `FileFollower` : `readAgain()` remontre ce qu'il tient (`showText()`, `m_shown`, document non modifié) si rien n'a été tapé, sinon le garde et lève `m_outdated` |

**Galaxy de NebulaStudio** (`StudioPlugin/`, issue #103) : `NebulaStudio.nbp` assemble la fenêtre de NebulaStudio à
partir des composants de `StudioPlugin` (catégories NebulaStudio et Layout) ; `NebulaStudio.exe` la fait tourner (issue
#73, `main.cpp`), NebulaStudio l'ouvre depuis l'onglet **Galaxies** et la lance par **Start** (une seconde fenêtre
NebulaStudio), et `ExampleUniverseApp scene/NebulaStudio.nbp` la fait tourner aussi. Ses plugins : `StandardPlugin` et `StudioPlugin`. Ses instances : `Studio`, `Edition`, `Project`, `History` (le
`HistoryComponent` de `StandardPlugin`, lié à `Edition.history`),
`Plugins`, `Galaxies`, `Components`, `Properties`, `Problems`, `Overview`, `Sources` (onglets des trois listes,
`Components` lié en premier), `EditionColumn` (splitter vertical : `Edition` au-dessus de `Problems`), `SideColumn`
(splitter vertical : `Sources` au-dessus de `Properties`, puis `Overview`, « 330, 240, 180 », `hiddenWhileDragging`),
`Columns` (splitter horizontal : `EditionColumn`, étirée, et `SideColumn`, « 1080, 360 ») et `Window`, dans quatre
zones.

| Composant | Rôle |
| --- | --- |
| `StudioPartComponent` | Base, non enregistrée, des parties faites d'un widget autour du `Studio` consommé (`studio`, `Mandatory`) : `createPart(studio)` (virtuelle pure) le construit quand le studio est là, fourni comme `widget` ; `withdrawPart()` retire ce que la sous-classe en fournit d'autre, puis la partie est détruite quand le studio part, ceux à qui elle était fournie l'ayant rendue ; `part()` |
| `StudioComponent` | Possède le `Studio` (préférences de `NebulaStudio`), fourni aussitôt (`studio`) ; à sa construction, `applyTheme()` une fois par processus, dans le schéma de ses préférences, `chooseColorScheme()` sur `colorSchemeChanged` de son `Studio` (la palette est celle du processus : le dernier choisi, dans n'importe quel `Studio`) et `applyCommandLine()` sur `QCoreApplication::arguments()` (les options d'`Universe`, `parse()` laissant de côté les autres) ; `onStop()` arrête l'application assemblée |
| `EditionZoneComponent` | `EditionZone` (`widget`, et `zone`), ses menus **Edit** (`editMenu`), **View** (`viewMenu`) et **Run** (`runMenu`), refaits avec elle, et `createZoomLabel()` (`zoomLabel`) ; consomme `history` (`Nebula::IHistory`, Optional, issue #132), donné à la zone (`giveHistory()`, `nullptr` quand il part : `onUnavailable` vient avant que le membre soit remis à `nullptr`) |
| `ProjectComponent` | Consomme `studio` et `zone` (`Mandatory` tous deux) : le `Project` (`project`) et son menu **File** (`fileMenu`), créés quand il a les deux, détruits quand l'un part ; propriété `file` (issue #73) : le projet ouvert à chaque démarrage (`onStart()`, puis `QTimer::singleShot(0)` : une fois la fenêtre montrée, qui démarre après lui), par `Project::openProjectFile()` après `saveChanges()`, s'il tourne encore et a son projet ; `main.cpp` lui donne celui de la ligne de commande |
| `PluginListComponent`, `GalaxyListComponent`, `ComponentListComponent` | `PluginPanel` (son `removing` → `EditionZone::remove()` de la `zone` consommée, s'il y en a une), `GalaxyPanel` (`openRequested` → `Project::openGalaxy()` du `project` consommé ; propriété `directory`, donnée au panneau, que `directoryChosen` change pendant l'exécution) et `ComponentPanel` |
| `PropertyEditorComponent`, `ProblemListComponent` | `PropertyEditor`, qui suit `selectionChanged` de la `zone` (`showSelection()` : le composant ou le fichier de réglages sélectionné ; `Mandatory` ; `setComponent(nullptr)` quand elle part), ses `refused` passés à `Studio::tell()`, et `ProblemList` (`componentActivated` → `EditionZone::showComponent()`, si elle a une `zone`) |
| `OverviewComponent` | Issue #64. Consomme `studio`, `zone` (`Mandatory`) et `viewMenu` ; la `SceneOverview` de `EditionZone::createOverview()` (`widget`), créée quand il a la `zone`, détruite quand elle part, après l'avoir retirée ; son entrée **Overview** (`m_entry`, cochable, cochée au départ) et un séparateur, des `QAction` à lui, ajoutés à la fin du `viewMenu` et retirés quand il part ; `toggled` montre ou masque la vue (`showAsChecked()`, seulement une fois qu'elle a un parent, sans quoi elle s'ouvrirait en fenêtre, et à nouveau dans `onStart()` : le splitter la remontre quand il la prend, avant que l'entrée soit cochée d'après le studio), `triggered` le dit au `studio` (`Studio::setOverviewShown()`), dont `overviewShownChanged` coche l'entrée |
| `StudioWindowComponent` | Sa `QMainWindow`, montrée pendant l'exécution (`width`, `height`, 1440×800), icône du logo, `createPluginDirectoryLabel()` ; consomme `centralWidget` (`Mandatory`), les listes `menus` (barre de menus refaite, `QMenuBar::clear()` ne détruisant que ses propres actions) et `statusWidgets` (widgets permanents, retirés puis remis dans l'ordre), `studio` (`told` → `tell()`) et `project` (titre, `titleChanged`) ; un filtre d'événements refuse la fermeture tant que `Project::saveChanges()` dit non |
| `SplitterComponent`, `TabsComponent`, `GroupComponent` | Mise en page de widgets quelconques, rendus sans parent à leur départ : `QSplitter` (liste `widgets`, `vertical`, `sizes` « 1080, 360 », `stretched` : l'index dont le facteur d'étirement vaut 1), `QTabWidget` (liste `pages`, `insertPage()`, `documentMode` ; la première page insérée est montrée), `QGroupBox` (`content`, `Mandatory`, `title` ; la galaxy ne s'en sert plus) ; `hiddenWhileDragging` du splitter et des onglets (issue #104) : leur `DragAside`, sur leur widget |

Les chaînes affichées passent par `tr()`, en anglais.

**Logo et icônes** (`NebulaStudio/resources/`) : `nebula.svg`, le logo de Nebula (une nébuleuse traversée de trois
astres liés), et `icons/{universe,galaxy,astre,comet,stargate}.svg`, une icône par terme du glossaire, sont des SVG aux couleurs
du thème, compilés par `nebulastudio.qrc` (AUTORCC, nommé dans `SOURCES`). Les icônes n'emploient que les couleurs de
zones qui gardent 3 contre 1 sur les champs des deux schémas (`#6366F1`, `#C026D3`, `#3B82F6`, `#EC4899`). Qt Svg est
lié à NebulaStudio (`find_package` dans son `CMakeLists.txt`) : `QSvgRenderer` dessine le logo sur le splash, et son
moteur d'icônes, déployé par windeployqt (`iconengines/qsvgicon`), lit les `QIcon` des SVG : `logoPath()` pour
`QApplication::setWindowIcon()` au début de `main.cpp`, `icon(GlossaryTerm)` de `Theme.h` pour **New** (Univers),
**Add zone**, l'onglet **Galaxies**, ses lignes et **Description...** (Galaxy), la liste des composants (Astre, `DecorationRole` de la colonne Name) et les fichiers de réglages
(Comet : **Add settings file**, **Edit settings file...**, `SettingsDialog`) et les stargates (Stargate, un anneau à
chevrons : **Add stargate**, **Expose...**, **Edit stargate...**, `StargateItem`, `StargateDialog`, les propriétés
exposées du `PropertyEditor`). `nebula.ico` (16 à 256 px, entrées PNG),
par `nebulastudio.rc` (`IDI_ICON1`, ajouté sous Windows seulement, à côté de la ressource de version de CMakeUtils), est
l'icône de `NebulaStudio.exe` : rendu du logo, à refaire quand il change, par un petit programme Qt qui le dessine avec
`QSvgRenderer` à chaque taille et assemble les PNG (pas d'outil dans le dépôt).

**Police** (`NebulaStudio/resources/fonts/`) : `SpaceGrotesk-Bold.ttf`, compilée dans l'exécutable par
`nebulastudio.qrc`, et sa licence `OFL.txt` (SIL Open Font License 1.1), que `release.yml` copie dans le paquet à côté
de NebulaStudio (`SpaceGrotesk-OFL.txt`). Seule la graisse Bold sert : en ajouter une autre, c'est l'ajouter au `.qrc`.

**Préférences** (`Preferences.h`, fonctions libres) : ce que NebulaStudio garde d'un lancement à l'autre pour
l'utilisateur, et qu'un projet ne tient pas, sur un `QSettings` : le style des liens, `readLinkStyle()` /
`writeLinkStyle()` (clé `edition/linkStyle`, le style par son nom : `curves`, `straightLines`, `rightAngles` ; absent ou
inconnu = courbes), et le schéma du thème (issue #118), `readColorScheme()` / `writeColorScheme()` (clé
`appearance/colorScheme` : `system`, `light`, `dark` ; absent ou inconnu = `system`, `Qt::ColorScheme::Unknown`), par
`readNamed()` / `writeNamed()` sur une table de `Named<Valeur>`, et si la vue d'ensemble est montrée (issue #64),
`readOverviewShown()` / `writeOverviewShown()` (clé `edition/overview`, un booléen : seul `false` la masque, tout le
reste vaut montrée). `preferencesFormat()` est un format JSON de
`QSettings` (`registerFormat()`, une fois par copie du code : celle de l'exécutable et celle de `StudioPlugin`) : un
objet, chaque groupe de clés un objet à lui (`addSetting()`, `withMember()`) ; un fichier qui n'est pas un objet n'est
pas lu (`FormatError`). Celui de NebulaStudio est `Studio::m_preferences`, construit dans ce format, en `UserScope`,
avec `organization` de `StudioPlugin_info.h` et le nom `NebulaStudio` par `StudioComponent` : le fichier
`FabienDG/NebulaStudio.json` du dossier des réglages de l'utilisateur, `%APPDATA%` sous Windows (voir Pièges) ; ce que
l'ancienne version gardait dans le registre n'est pas relu. `TestPreferences` écrit dans un fichier JSON temporaire,
et `TestStudioPlugin` met les réglages de l'utilisateur dans un dossier temporaire (`QSettings::setPath()` du format
ini, celui des formats qui n'ont pas le leur).

**Fichier de projet `.nbp`** (JSON, version 1, format décrit dans le README)
- Le contenu applicatif est lu et écrit par `Universe` (voir Core), `description` comprise, que **File** >
  **Description...** édite, les galaxies posées (`type` `Nebula.Galaxy`, `galaxy` relatif au fichier du projet,
  `properties` : valeurs de ses propriétés exposées) et les stargates (`type` `Nebula.Stargate`, `provided`,
  `consumed`, `exposed`).
  `EditionScene::save()` y ajoute `x` / `y`
  et `color` (`#aarrggbb`, seulement si une couleur a été choisie) dans chaque composant (retrouvé par son nom), et
  `sides` (`{"consumed": {"content": "right"}, "provided": {…}}`, seulement les points qui ne sont pas sur le bord de
  leur genre, relus par `readSides()` avant le tracé des liens ; un bord inconnu vaut celui du genre) et `rows`
  (`[{"provided": "display"}, {"consumed": "content"}]`, l'ordre d'`interfaceRows()`, seulement s'il n'est pas celui
  des genres ; relu par `readRows()`, une ligne sans genre connu ou d'une interface absente laissée de côté),
  `x` / `y` dans chaque fichier de `settings` (par rang, dans l'ordre de `settingsFiles()`), `anchors` dans chaque
  entrée de `bindings` qui en a (`LinkAnchors::written()`, ignoré par Core) et `zones` ;
  `EditionScene::load()` retire ses items, appelle `Universe::load()`, puis recrée un `ComponentItem` par
  composant, un `LinkItem` par `binding()`, avec ses ancres, et un `SettingsItem` par fichier de réglages.
- `Project::openProjectFile()` : `Universe::read()`, puis `EditionScene::load()`, puis relit plugins et galaxies.
  Les problèmes sont affichés dans une seule boîte ; après une ouverture partielle, le projet n'a plus de fichier
  associé pour que **Save** n'écrase pas ce qui manque.
- « Modifié » n'est pas suivi au fil de l'eau : `Project::saveChanges()` compare `save()` de la scène de
  l'application à `m_savedProject`, mémorisé par `setProjectFile()`. Toute nouvelle donnée à sauvegarder doit donc passer par `EditionScene::save()`.
- `NebulaStudio [--plugin-dir <dossier>]... [--plugin-root <dossier>] fichier.nbp` ouvre le projet au démarrage : les
  options, déclarées par `NebulaApplication`, sont appliquées à l'application qui fait tourner la galaxy, et, par
  `StudioComponent` (`Studio::applyCommandLine()`), à celle qu'édite NebulaStudio ; le fichier va à `Project.file`.
- Les dialogues partent de `scene/` à côté de `NebulaStudio.exe` (`defaultProjectDirectory()` de `ProjectFiles.h`, créé
  au premier **Save As...**), ou du dossier du fichier du projet quand il en a un.
- Menu **Run** (`EditionZone::addRunActions()`) : **Start** (F5) valide d'abord un nom en cours d'édition
  (`setFocusItem(nullptr)`), appelle `Universe::start()` et affiche `errorString()` en cas de refus. **Stop** (Maj+F5) appelle `stop()`. Leur
  activation suit `runningChanged`, comme celle de **New** et **Open...**, désactivés pendant l'exécution. Comme
  `stop()` remet les valeurs d'édition et que `save()` les écrit pendant l'exécution, faire tourner l'application ne
  rend pas le projet « modifié ».

## Pas encore fait

- Annulation (#132) : un composant supprimé l'est sans confirmation, mais **Undo** le rend. Une étape garde deux
  snapshots de toute la scène (100 étapes par défaut) ; quand le contenu diffère (composants, liens, propriétés,
  fichiers de réglages), l'application est rechargée en entier (`load()`) ou la galaxy rendue : ses composants sont
  refaits, ce que montrent les parties aussi (seuls les noms sélectionnés le restent), et les galaxies ouvertes se
  ferment quand c'est l'application. Une étape de l'application défaite pendant qu'une galaxy est montrée la change sans
  la montrer : seule la barre d'état le dit. Les étapes d'une galaxy quittée sont perdues (obsolètes), même enregistrée.
  Le menu ne dit pas ce que **Undo** défera, ni s'il reste quelque chose (hors périmètre). Les valeurs d'un fichier de
  réglages (inspecteur, texte), le thème, le style des liens et le zoom ne s'annulent pas. Ctrl+Z dans un champ n'est
  vérifié qu'à la main (les raccourcis ne se testent pas `offscreen`) ; seule la garde `isTyping()` couvre Ctrl+Maj+Z.
- Copier-coller (#136) : ni les stargates (leurs noms exposés sont uniques dans la galaxy), ni les fichiers de
  réglages, ni les zones ne se copient, et un lien vers une instance non copiée ne se garde pas (la copie n'est pas liée
  au même fournisseur). **Cut** supprime toute la sélection, comme Suppr, ce qu'il ne copie pas compris (un fichier de
  réglages, une stargate, un lien vers une autre instance). Pendant une saisie (un champ, un nom édité), les entrées du
  menu **Edit** ne font rien, depuis le menu non plus, comme **Undo** : elles n'agissent pas sur le texte du champ, seuls
  ses raccourcis le font. Ctrl+C, Ctrl+V et Ctrl+D dans un champ ne sont vérifiés qu'à la main (raccourcis non testables
  `offscreen`). Le décalage compte les collages du même contenu même défaits ; un collage n'est pas aligné sur la
  grille ; le coin de **Paste** du menu contextuel ne tient pas compte des points d'interface qui dépassent des bords.
  Le presse-papiers ne tient que le format de NebulaStudio : rien ne se colle en texte ailleurs, et du JSON copié en
  texte depuis une autre application ne se colle pas (hors périmètre). Un collage dans une autre fenêtre de
  NebulaStudio n'est vérifié qu'à la main.
- Aucun composant d'exemple ne refuse de démarrer : le chemin d'erreur de `start()` n'est exercé que par les tests
  (`Probe`), pas dans NebulaStudio.
- Le verrouillage pendant l'exécution n'est complet que dans NebulaStudio : Core refuse `instantiate()`, `rename()` et
  `remove()`, mais pas `Component::bindInterface()` ni les changements de fichiers de réglages.
- Tests : rien pour le `main.cpp` de NebulaStudio (ni pour `EditionZone`, `Project` et les panneaux, que `TestStudioPlugin` n'exerce
  qu'à travers la galaxy de NebulaStudio), la scène et ses items hors de ses liens (`TestEditionScene`), les fenêtres
  (`SettingsDialog` hors de son nom, de ses cases, de son filtre et de ce qu'elle applique, `TestSettingsDialog`,
  `StargateDialog` hors de ses cases, des noms qu'elles donnent, de son filtre, des noms refusés et de ce qu'elle
  applique, `TestStargateDialog` : ni son en-tête dans une galaxy, ni le double-clic, ni une liste qu'une autre
  stargate expose, et `PropertyEditor` hors de ses groupes et de ses éditeurs, `TestPropertyEditor`), ni `NebulaPlugin.cmake`. Le paquet n'est vérifié par rien : ni projet tiers d'exemple dans le dépôt, ni étape de CI qui
  en compile un.
- Composants non opérationnels : **Start** ne les signale pas et ne refuse rien ; seuls le « ! », l'ambre et
  `ProblemList` les montrent.
- Style des liens (#84) : un seul pour toutes les scènes ; les pointillés des fichiers de réglages et des stargates
  restent droits. Aucun style n'évite les
  composants : un trait droit, ou un lien à angles droits entre deux composants à la même hauteur, peut passer dessous.
- Thème (#118) : clair, sombre ou celui du système, rien d'autre (ni couleurs ni police à choisir), et seulement par
  **View** > **Theme** : pas de fenêtre de préférences. Il vaut pour tout le processus : choisi dans la galaxy de
  NebulaStudio lancée depuis NebulaStudio (**Start**), il change aussi la fenêtre qui l'édite, dont le menu ne coche
  pas le nouveau (chaque `Studio` garde le sien). Le fichier des préférences n'est lu qu'au lancement : changé à la
  main, NebulaStudio ouvert, il ne change rien avant le lancement suivant.
- Recherche des instances (#63) : seulement dans ce que montre la zone d'édition, pas dans les galaxies posées (il
  faut ouvrir la galaxy), ni par une propriété ou une interface ; la liste des composants ne dit pas si un composant est
  instancié. Rien ne marque dans la scène toutes les instances trouvées : seule celle montrée est sélectionnée.
- Vue d'ensemble (#64) : toujours toute la scène (4000×3000), même quand ses instances n'en occupent qu'une partie, où
  elles sont alors petites ; elle ne zoome pas, ni la zone d'édition depuis elle (pas de molette), et ne montre pas
  l'intérieur des galaxies posées. Elle est sous les propriétés, sans titre : rien ne la pose sur la zone d'édition ni
  ne l'en détache. Le splitter ne garde pas sa taille d'un lancement à l'autre, comme les autres.
- Liens suggérés (#82) : **Bind to** n'est que sur une interface consommée ; le menu d'une interface fournie ne propose
  pas les consommées qui pourraient s'y lier. Seules les interfaces de la scène montrée y sont.
- Ancres des liens (#106) : elles restent à leur place quand on déplace des composants, une sélection ou une zone, et ne
  s'alignent pas sur la grille ; le lien en cours de tracé n'en a pas. Renommer une interface d'une stargate perd les
  ancres de ses liens (la clé de `LinkAnchors` est le nom). Rien ne marque une ancre sélectionnée : elle ne se
  sélectionne pas, et Suppr ne la retire pas.
- Points de dérivation (#126) : un projet ne les écrit pas à part ; deux liens d'une même interface fournie dont les
  ancres commencent aux mêmes positions passent par les mêmes points à la relecture, même s'ils ne les partageaient
  pas (deux ancres posées exactement au même endroit, ce que rien n'empêche). On ne dérive un lien que vers une
  interface consommée, depuis le côté du fournisseur : rien ne réunit plusieurs fournisseurs sur un trait. **Bind to**
  (#82) trace toujours un lien à part. Un point de dérivation qui ne l'est plus (il ne reste qu'un lien) devient une
  ancre, traversée alors dans le sens du point d'avant vers celui d'après : le trait passe par les mêmes points, mais
  peut s'y courber autrement. Retirer un point de dérivation retire aussi, des liens qui y passaient, les ancres d'avant
  lui, même celles qu'avait le lien d'origine. Les pointillés des fichiers de réglages et des stargates ne dérivent
  pas.
- Bord et ligne des points (#117) : à gauche ou à droite seulement, ni en haut ni en bas ; une ligne par interface, deux
  lignes s'échangent mais rien ne déplace une ligne d'un coup au-dessus d'une autre. Seul le menu de Maj + clic droit
  les change : rien ne fait glisser un point, et rien ne remet d'un coup ceux d'un composant. Une interface d'une
  stargate changée par sa fenêtre (retirée puis exposée de nouveau) revient sur le bord de son genre et en bas ; seul le
  renommage sur place garde l'un et l'autre (les clés sont les noms).
- Listes d'interfaces (#67) : leur ordre est celui des liaisons, que rien ne change sans délier puis relier, et que
  NebulaStudio ne montre pas ; la relecture d'une galaxy relie à la fin d'une liste ce qui la liait à elle. Seul un
  composant consomme une liste : une galaxy (donc une stargate) consomme ou fournit une interface seule, qu'une liste
  peut avoir parmi d'autres. Une même liste ne se lie pas deux fois à la même interface fournie.
- Interfaces et objets qui suivent une propriété (#86) : `stop()` ne refait que les valeurs. Quand la propriété change
  pendant l'exécution (un fichier de réglages qui la tient, ou le composant lui-même), ce qu'elle retire perd ses
  liens, et ce qu'elle remet à l'arrêt est neuf : les valeurs d'édition de ses objets (mémorisées par `QPointer`) sont
  perdues, et le projet est alors « modifié ». Le README conseille de ne pas mettre une telle propriété dans un fichier
  de réglages ; rien ne l'empêche. Une interface ajoutée ou retirée refait tous les `InterfaceItem` de son composant
  (`clearIfChanged()`), ses liens redessinés par `refreshLinks()`.
- Galaxies : une galaxy éditée ne passe à ses autres instances qu'une fois enregistrée (**Save Galaxy**, ou en la
  quittant), et un fichier de galaxy changé hors de NebulaStudio n'est relu qu'à l'ouverture du projet ; le chemin ne
  marque pas une galaxy changée. La description d'une galaxy ne s'édite qu'en ouvrant son fichier comme projet
  (**Description...** est celle de l'application). Les valeurs des propriétés exposées de l'instance éditée deviennent
  celles du fichier. Une interface qu'une stargate fait consommer à sa galaxy depuis qu'elle est posée reste `Optional`
  jusqu'à la relecture. Les problèmes de ses
  composants (interfaces obligatoires non liées, interface d'une stargate liée à rien) ne remontent pas dans
  `ProblemList`. Un fichier de réglages de l'application ne tient pas une propriété exposée par une galaxy (seulement
  ceux de la galaxy, sur ses composants). L'onglet **Galaxies** ne dit pas ce qu'une galaxy expose avant qu'elle soit
  posée, et une galaxy posée ne montre ses propriétés que dans l'éditeur de propriétés, pas sur son item.
  **Expose...** pose la stargate sans éviter ce qui est déjà là. Les galaxies ne sont pas dans la liste des composants
  (#42). Un plugin connu n'est pas vérifié contre les types des composants (il faudrait le charger) ; le dossier de
  l'onglet n'est pas mémorisé d'un lancement à l'autre ; la galaxy ouverte n'est pas marquée dans la liste. Le terme
  Galaxy reste aussi sur les zones (`ZoneItem`, **Add zone**).
- Exemples : `Greeter`, `Ticker`, `ProfileSelector` et `Window` gardent leur nom et leur classe `*Logic`, que le
  composant crée désormais ; seuls ceux de `WidgetsPlugin` suivent la bonne pratique (`<Nom>Component`, objets
  membres). Aucun projet d'exemple n'assemble `WidgetsPlugin` dans le dépôt.
- Plugins standard (#130) : un seul composant, `LogComponent`. Pas de fichier de log à lui, pas de vue des messages, pas
  de composant qui choisit le niveau (une liste qui consommerait `logControl`), et ni NebulaStudio ni un projet
  d'exemple ne s'en servent. Un seul niveau pour tout le processus, ni par catégorie ni par plugin, et `level` est une
  chaîne : l'éditeur de propriétés n'en propose pas les niveaux, et en faire une énumération changerait ce qu'écrivent
  projets et fichiers de réglages (`debug` deviendrait `Debug`). `ILogControl` ne prévient de rien. On n'écrit que depuis le
  thread du composant : changer `category` pendant qu'un autre thread écrit n'est pas protégé. Deux instances en cours
  n'ont qu'un niveau : la dernière démarrée ou pilotée impose le sien, qui passe aussi dans les fichiers de réglages
  des autres. Lancé par **Start** dans NebulaStudio, il filtre aussi les messages de NebulaStudio, le même processus.
- Galaxy de NebulaStudio (#103, #73) : le splash ne se montre qu'une fois la galaxy chargée (`NebulaApplication` charge
  dans son constructeur, et n'offre aucun crochet avant), et `main.cpp` trouve le `Project` par le nom de son instance :
  une galaxy enregistrée sans lui n'ouvre pas le projet passé en argument, sans rien en dire. Sa fenêtre
  ne demande d'enregistrer qu'à sa fermeture : arrêtée (**Stop**) ou détruite avec l'application qui la fait tourner,
  elle perd ce qui n'est pas enregistré. Une partie qui perd puis retrouve son `studio` est refaite à neuf : l'état de
  ses listes (sélection, recherche) est perdu. Dans le paquet, la galaxy et la licence de la police ne sont ajoutées
  que par `release.yml`, que rien d'autre ne vérifie.
- Nom du paquet et détails des binaires (#139) : `release.yml` ne publie que sur un tag `X.Y.Z` égal à la version de
  `project()` : un `-rc0` se construit, mais ne se publie pas. Seule la CI Windows construit le paquet : les noms des
  autres plateformes et compilateurs (`linux`, `gcc14`) ne sont vérifiés que par `TestPackageName`, sur des valeurs
  données. Windows montre la description d'un exécutable comme son nom (gestionnaire des tâches, « Ouvrir avec ») :
  plateforme, compilateur et Qt compris.
- Disposition de NebulaStudio (#104) : le côté droit (listes et propriétés) ne se masque que le temps d'un glisser qui
  part de lui ; rien ne le replie à la demande (on le réduit en tirant la poignée du splitter), et les tailles des
  colonnes ne sont pas gardées d'un lancement à l'autre. Masqué, il perd le clavier, que Qt donne au widget suivant (le
  champ qui trouve les instances) : rien ne le rend à la liste après un glisser abandonné. Les tests simulent le glisser par les
  signaux de `DraggableView` et les événements de la fenêtre : un vrai glisser (OLE sous Windows) n'est vérifié qu'à la
  main.
- Fichiers de réglages : pas de fenêtre « Préférences » générique où l'utilisateur final éditerait tout un fichier
  (chaque composant montre les siens), pas de partage d'une même valeur entre deux instances, et le cas de plusieurs
  objets de même nom dans un composant (`index` d'une entrée) n'est exercé que par les tests (`Panel`). Sa fenêtre
  (#105) ne filtre que par les noms, ni par les valeurs ni par les types ; **Held only** ne se refait qu'au changement du
  filtre, pas quand on décoche (la ligne resterait sinon sous le pointeur). De même pour `StargateDialog` (#116) et
  **Exposed only** : son filtre ne cherche pas dans les noms exposés (colonne Exposed as) ; cocher un composant expose
  tout ce qu'il a, interfaces comprises, chacune sous le nom de son interface (numéroté s'il est pris), même cachée
  par le filtre ou dans un groupe fermé.
- Propriétés typées (#135) : ni listes, tableaux et maps (#27), ni `QPoint`, `QSize`, `QRect`, `QFont`… Une couleur ne
  se tape pas : seule sa fenêtre la choisit, ouverte par un double-clic, F2 ou Espace, pas par un clic comme les autres
  valeurs. Le bouton d'une URL ne choisit qu'un fichier existant, ni un dossier ni une adresse distante, et une
  propriété `QString` qui serait un chemin n'a pas de bouton. Une case de la liste des drapeaux se coche par sa case ou
  Espace, pas en cliquant sur son nom. La fiche d'un composant (`ComponentViewer`) ne liste pas les valeurs d'une
  énumération. La fenêtre native de choix d'un fichier (Windows) n'est vérifiée qu'à la main : les tests passent par
  celle de Qt (`offscreen`).
- Valeurs d'un fichier de réglages (#133) : pendant l'exécution, elles ne sont que montrées, et ce qu'un autre
  programme écrit alors dans le fichier n'est pas donné à l'application qui tourne, qui l'écrase à sa prochaine
  écriture. L'inspecteur ne montre que les entrées du fichier ; le reste de son contenu (d'autres instances, une autre
  application) ne se voit que dans son texte (**View and edit file...**), et une valeur dont le type n'a pas d'éditeur
  ne s'y change qu'à la main. La fenêtre du texte n'a ni coloration ni mise en forme : elle écrit ce qui est tapé. Un
  dossier de fichier qui n'existe pas encore n'est suivi qu'une fois créé par NebulaStudio ou l'application (le
  `FileFollower` est remis à chaque relecture) : un autre programme qui le crée n'est pas vu avant.
- Analyse SonarQube Cloud (#147) : le plan Free n'a que la quality gate Sonar way, qu'on ne copie ni ne change (ni
  ses 80 % de couverture, ni ses règles : le profil Sonar way du C++, qui ne suit ni `CONVENTIONS.md` ni
  `.clang-tidy`) ; le plan OSS, pour un projet public sous licence libre, le permettrait. La couverture n'est que celle
  des tests de `tests/`, sous Windows, d'un Release optimisé (`/O2 /Ob2`) : une fonction inlinée n'a pas toujours ses
  lignes, et une PR qui change ce que les tests n'exercent pas (voir Tests ci-dessus) ne passe la gate qu'avec des
  tests. OpenCppCoverage n'a plus de version depuis 0.9.9.0 (2020). Rien ne lance l'analyse en local ni avant la PR.
  Une PR depuis un fork n'a pas le secret : la CI s'arrête, comme pour Qt. Aucune règle de `main` n'exige la CI verte :
  une PR rouge reste fusionnable sur GitHub.

## Pièges

- Modifier `Component.h`, `IComponent.h` ou `Plugin.h` change l'agencement binaire : tous les plugins doivent être
  reconstruits, y compris ceux ouverts par **Open plugin...**, sinon plantage à la création d'un composant. Le contrôle
  de version ne compare que la majeure de Core.
- La zone d'édition (ses items) et l'éditeur de propriétés référencent les composants de l'application d'un `Studio` :
  dans la galaxy, chaque partie est détruite quand elle perd son `studio` (`StudioPartComponent`), donc avant lui ;
  garder cet ordre quand une partie est ajoutée.
- Un `QGraphicsTextItem` qui a le focus fait défiler la vue pour montrer son curseur, et la vue rejoue alors le dernier
  événement souris comme un déplacement : un item encore tenu par le double-clic qui a lancé l'édition suivrait le
  défilement. `ComponentItem::mouseMoveEvent` ignore donc les déplacements tant que son nom est édité.
- `NebulaPlugin.cmake` s'exécute aussi chez un projet tiers, inclus par `find_package` depuis le paquet :
  `PROJECT_NAME` et `PROJECT_VERSION` y sont ceux de ce projet, `CMAKE_CURRENT_LIST_DIR` le dossier installé. La
  version de Core (`coreVersion`) y vient donc de `Nebula_VERSION`, et rien ne doit y lire l'arbre de sources. Un
  changement de ce fichier se vérifie avec un projet jetable qui fait `find_package` sur `<build>/package`.
- La palette de l'application change avec le schéma de couleurs du système, NebulaStudio ouvert : ce qui peint lit la
  palette au moment de peindre, et ce qui en garde une couleur (le fond des titres de groupes du `PropertyEditor`) la
  remet sur `QEvent::PaletteChange`. Une couleur en dur, hors des jetons de `Theme.cpp`, ne suit pas le schéma et n'est
  pas vérifiée contre les WCAG.
- Le style de l'application est `ThemeStyle`, un `QProxyStyle` de Fusion : `QApplication::style()->name()` est vide,
  c'est son `baseStyle()` qui s'appelle `fusion`. Il redessine les bords des cases à cocher et des champs par-dessus
  ceux de Fusion, au même endroit (décalés d'un demi-pixel, comme Fusion) : une version de Qt qui changerait ce tracé
  ferait apparaître un double trait, que `TestTheme::edges()` ne voit pas (il ne lit que le milieu du bord gauche).
- Sur la plateforme `offscreen` des tests, sans polices, `QFontInfo` renvoie une taille négative, que
  `QFont::setPointSizeF()` refuse en avertissant : `DialogHeader` agrandit son titre depuis la taille de la police
  elle-même, en points ou en pixels.
- Un composant dérive publiquement de `Component` (`std::is_base_of_v` accepterait un héritage privé, refusé plus
  loin, à la création, sans message), ne peut pas être `final`, a besoin d'un constructeur par défaut et de `Q_OBJECT`.
- Une propriété `QDate`, `QTime` ou `QDateTime` nulle ne fait pas l'aller-retour JSON (`ProjectJson::toJson()`) : tant
  qu'elle l'est, un projet ne l'écrit pas et `SettingsFile::canHold()` la refuse. Les composants de test ont donc des
  dates valides par défaut. De même pour une `QColor` invalide, ou qui n'est pas en RGB sur 8 bits par canal (HSV,
  flottants : son nom l'arrondit) ; celles de `QColorDialog` le sont toujours, et la couleur de `Probe` aussi. Ses
  conversions sont celles de QtGui, chargé par le plugin qui a la propriété : Core ne le lie pas.
- moc ne produit pas de code qui compile pour une propriété de drapeaux (`Q_FLAG`) déclarée `MEMBER` (Qt 6.11) : elle
  a `READ` et `WRITE`, comme `styles` de `Probe`. Des drapeaux dont l'énumération est aussi déclarée par `Q_ENUM` font
  trouver à Qt l'énumération plutôt que les drapeaux pour les convertir en texte : des valeurs combinées n'y ont pas de
  nom, et un projet ne les écrit pas (vérifié avec Qt 6.11). Seul `Q_FLAG` les déclare ; `metaEnumOf()` de NebulaStudio
  trouve les drapeaux dans les deux cas.
- Sous Windows, sans console, Qt n'envoie ses messages (le journal de QTest compris) qu'au débogueur :
  `QT_FORCE_STDERR_LOGGING=1` les fait écrire sur la sortie d'erreur, ce que `nebula_add_test()` donne à CTest.
- `Ticker` et `Window` ont besoin d'un `QApplication` (widgets) : pas utilisables dans `ExampleApp`, pas plus que les
  composants de `WidgetsPlugin`. `ExampleApp` charge tout `plugins/`, donc `WidgetsPlugin` aussi, sans l'instancier ;
  dans le paquet, son dossier n'a pas `Qt6Widgets` (windeployqt ne voit que l'exécutable) : il dit que ce plugin n'est
  pas chargé et continue.
- Un objet qu'un composant prend d'un autre par une interface (un widget qu'il place, une barre ajoutée à une fenêtre)
  lui est rendu sans parent dans `onUnavailable` (`setParent(nullptr)`, `takeCentralWidget()`, `removeToolBar()`) :
  sinon le parent qui le tient le détruirait avec lui, alors que c'est le membre d'un autre composant. Une
  `QToolBar` possède les widgets qu'on y ajoute (`addWidget()`) : `TopBarComponent` y met des supports à lui, où il
  range les widgets consommés.
- Tout ce qu'un objet ajouté par `addPropertiesForQObject()` a d'inscriptible est enregistré : pour l'afficheur du
  `Ticker`, `value` compris, qui change à chaque tic, et que `stop()` remet donc à sa valeur d'édition (`ticks`, en
  lecture seule, n'est pas remis). C'est aussi ce que propose la fenêtre d'un fichier de réglages : toutes les
  propriétés d'un `QWidget`. `LabelsComponent` ajoute donc pour chaque label un `Label`, `QObject` qui n'a que son
  `text`, plutôt que le `QLabel` lui-même.
- Un fichier de réglages range les valeurs sous le nom des instances : renommer une instance laisse ce que
  l'utilisateur lui avait donné sous l'ancien nom, et elle repart des valeurs du projet. Lancée depuis NebulaStudio, une
  application dont le fichier n'a pas de dossier écrit dans `AppConfigLocation` de NebulaStudio, pas dans celui de
  l'application livrée.
- NebulaStudio ne fixe pas `QCoreApplication::setOrganizationName()` : ce nom entre dans `AppConfigLocation`, et le
  fixer déplacerait les fichiers de réglages qu'y écrivent les applications lancées depuis NebulaStudio. Ses
  préférences donnent donc l'organisation et le nom au constructeur de `QSettings` (`Studio::m_preferences`) ; un
  `QSettings` construit sans eux n'y lit ni n'écrit rien.
- Deux `Universe` dans un même processus ont chacune leur `PluginManager` mais
  partagent `ComponentFactory::instance()` : la seconde instancie des composants dont elle ne connaît pas le plugin,
  qui manque alors dans le projet enregistré. Ces instances survivent aussi au plugin quand la première le décharge
  ou est détruite : un `PluginManager` ne garde en vie que ce qu'il a chargé lui-même, et ne décharge que cela.
- `QAbstractItemModel::checkIndex()` avertit (« is not valid (expected valid) ») quand on lui passe un index invalide
  avec `IndexIsValid` : `flags()`, que les vues et `QAbstractItemModelTester` appellent avec l'index racine, teste
  `isValid()` d'abord, sans quoi `QTest::failOnWarning()` fait échouer les tests du modèle.
- Un widget ajouté à un layout dont le parent est déjà visible n'est montré qu'au tour suivant de la boucle
  d'événements : le chemin au-dessus de la zone d'édition, refait à chaque niveau, n'est visible qu'après
  `processEvents()` dans un programme qui pilote la fenêtre. Ses boutons sont détruits par `deleteLater()` : l'un d'eux
  est celui qui vient d'être cliqué.
- Un `QFileSystemWatcher` sur un dossier signale qu'un fichier y est ajouté, retiré ou renommé (donc un enregistrement
  par `QSaveFile`), pas qu'un fichier existant est réécrit sur place : `GalaxyModel` suit aussi chaque fichier listé,
  et les remet tous à chaque `refresh()` (un fichier remplacé est oublié du watcher). `FileFollower` fait de même pour
  un seul fichier et son dossier.
- Un plugin déchargé garde sa DLL en mémoire : `QPluginLoader` charge avec `PreventUnloadHint` par défaut, et il faut
  le laisser. Sans lui, la DLL part vraiment, mais le registre des métatypes de Qt garde des pointeurs vers les types
  du plugin (`QMetaType::fromName("IGreeter*")` plante après coup) et un `QVariant` lu d'une de ses propriétés pointe
  aussi dans la DLL. Conséquences : le fichier `.nbdl` reste verrouillé jusqu'à la fermeture, et un plugin recompilé
  n'est relu qu'au lancement suivant.
- `cmu_add_target`, donc `nebula_add_plugin()`, met toute cible qu'il crée dans le paquet, par la propriété globale
  `CMU_PACKAGE_TARGETS` de CMakeUtils, lue à la fin de la configuration. `nebula_add_test_plugin()` en retire chaque
  plugin de test : si CMakeUtils change cette mécanique, ils entrent dans le paquet sans un mot, ce que montre
  `install_manifest_cmu_package.txt` dans le build.
- Le type d'une interface d'un plugin n'est enregistré sous son nom qu'une fois utilisé : `QMetaType::fromName()` ne
  le trouve pas forcément à la lecture d'une stargate. `Universe::interfaceType()` le cherche donc d'abord parmi les
  interfaces des composants déjà créés, dont il est le type.
- Les interfaces d'une stargate se renomment et se retirent : un `ComponentItem` refait alors ses `InterfaceItem`, et
  leurs `LinkItem` partent avec eux ; c'est `EditionScene::refreshLinks()`, sur `interfacesRebuilt`, qui les redessine
  d'après les liens de Core, avec les ancres que `LinkAnchors` a gardées (un `LinkItem` ne peut pas les garder : il
  est détruit). Un nouveau chemin qui lie ou délie hors de `EditionScene::link()` et `unlink()` doit
  l'appeler aussi.
- La scène que montre la zone d'édition a deux vues, celle de la zone et sa vue d'ensemble (#64) : la vue de la zone
  change de scène par `EditionView::showScene()`, jamais `setScene()`, que la vue d'ensemble ne verrait pas. Elle reste
  ainsi la première de `QGraphicsScene::views()`, sur laquelle `EditionScene` ouvre la fenêtre d'une stargate. Et un
  test qui cherche la vue de la zone la cherche dans l'`EditionZone` : `findChild<QGraphicsView*>()` sur la fenêtre
  trouverait d'abord la vue d'ensemble, moins profonde.
- `QGraphicsScene` donne les événements de survol à l'item le plus haut qui les accepte, même sous un item qui ne les
  accepte pas : un `LinkItem` est survolé sous un `ComponentItem`. Il ne se dit donc survolé que s'il est l'item le plus
  haut à cet endroit (`itemAt()`).
- `StudioPlugin` recompile les sources de `NebulaStudio/src/` (un `GLOB_RECURSE`) : une nouvelle source y entre
  d'elle-même, et une qui inclut `NebulaStudio_info.h`, celui de l'exécutable (`SplashScreen`), doit être
  retirée de la liste dans `StudioPlugin/CMakeLists.txt`. Quand NebulaStudio charge le plugin, ses classes sont en deux
  exemplaires, chacun avec ses statiques (`applyTheme()` et ses ressources, rappelés par le premier `StudioComponent`) :
  rien ne passe de l'un à l'autre, et les interfaces `Studio`, `EditionZone` et `Project` ne se lient qu'entre les
  composants du plugin.
- La galaxy de NebulaStudio a son `Universe`, celle de son `StudioComponent`, à côté de celle de l'application qui la
  fait tourner, la `NebulaApplication` de `NebulaStudio.exe` comprise (voir « Deux `Universe` ») : elle liste
  `StudioPlugin` comme chargé, par l'autre gestionnaire (le `QPluginLoader` est partagé), ce que son **Unload** refuse,
  et ses composants parmi les autres.
- `scene/NebulaStudio.nbp` est la copie de `StudioPlugin/NebulaStudio.nbp`, refaite à chaque build (`copy_if_different`) :
  ce qu'on y enregistre se perd au build suivant, mais c'est elle que `NebulaStudio.exe` fait tourner d'ici là : une
  galaxy cassée là donne un NebulaStudio cassé, ou une boîte d'erreur, jusqu'au build suivant (dans le paquet, rien ne
  la refait). Ses composants sont placés vers le milieu de la scène (4000×3000), là où la vue s'ouvre.
- `NebulaApplication::exec()` cache `QCoreApplication::exec()`, statique : un appel par la classe de base
  (`QApplication::exec()`) fait tourner la boucle sans démarrer la galaxy. Ce qu'une `NebulaApplication` donne à
  `QCoreApplication` (nom, version, organisation) et le gestionnaire de messages sont globaux au processus : ils
  restent pour la suivante (`TestNebulaApplication::cleanup()` les remet), et ses règles de filtrage
  (`QLoggingCategory::setFilterRules()`) remplacent celles qu'un `main` aurait posées, et sont vidées à sa destruction.
- `QLoggingCategory::installFilter()` passe le nouveau filtre sur chaque catégorie avant de rendre le précédent : un
  filtre qui appelle celui d'avant (`LogFilter::filter()`, celui de `TestLogComponent`) ne fait rien tant qu'il ne le
  connaît pas, sans quoi il appelle un pointeur nul, puis se fait repasser (`LogFilter::refilter()`). Un filtre de
  catégories est global au processus et les filtres se chaînent : on ne retire le sien que s'il est en haut de la
  chaîne. Les règles (`setFilterRules()`, celles de `NebulaApplication`) passent, elles, par le filtre installé, donc par
  celui d'un `LogComponent` en cours.
- NebulaStudio est un exécutable : ses tests recompilent ses sources, listées dans `tests/NebulaStudio/CMakeLists.txt`.
  Un nouveau `.cpp` qu'utilisent la scène, ses items ou ses fenêtres s'ajoute à `_scene`, les `SOURCES` de
  `TestEditionScene`, de `TestComponentFinder` et de `TestStargateDialog`, sans quoi l'édition de liens échoue (un
  en-tête seul, comme `SceneItems.h`, n'a rien à y ajouter) ; un qu'utilise `SettingsDialog`, à ceux de
  `TestSettingsDialog`, comme `ChoicePanel.cpp` ; un qu'utilise `SettingsValueEditor`, à `_settings_values`, les
  `SOURCES` de `TestSettingsValueEditor` et de `TestPropertyEditor`.
- Le `PropertyEditor` a deux tables, celle d'un composant et celle d'un fichier de réglages, dans un `QStackedWidget`
  qui met la page montrée en dernier parmi ses enfants (`raise()`) : `findChild<QTableWidget*>()` ne trouve pas
  toujours la même. `TestPropertyEditor` prend celle qui n'est pas sous le `SettingsValueEditor` (`isAncestorOf()`).
- `Universe::reloadGalaxy()` détruit les composants et les fichiers de réglages de la galaxy, pas l'objet `Galaxy` :
  ce qui montre ceux-là les lâche sur `Galaxy::aboutToBeReloaded` (la scène d'une galaxy le fait), ce qui montre
  celle-ci (son item, ses liens) reste. Parmi les scènes ouvertes, seule celle de l'instance éditée peut montrer le
  contenu d'une instance relue : toute autre serait dans une galaxy qui se contient. `leaveGalaxies()` ferme donc la
  scène d'une galaxy avant de la relire, ou de relire les instances de son fichier : celles du dessous montreraient le
  contenu détruit.
- Toute action ajoutée à l'éditeur passe par l'historique (#132) : un geste de la souris, une entrée de `ContextMenu`
  ou Suppr y sont déjà ; autrement, `EditionScene::edit()` (ou `EditionZone::edit()` hors de la scène), ou un
  `snapshot()` avant puis `record()`. Une action qui n'y passe pas finit dans l'étape suivante, qui la défait avec elle.
  Ce qu'une étape rend vient de `EditionScene::save()` : une donnée nouvelle de la scène doit y être écrite et relue par
  `show()`, sinon elle n'est ni gardée ni rendue. Une modification du projet qui n'est pas une édition (relecture d'un
  fichier, déchargement d'un plugin, autre projet) vide l'historique ou laisse ses étapes obsolètes.
- Ce que copie la scène (#136) passe par les mêmes fonctions que `save()` (`placeComponents()`,
  `LinkAnchors::written()`) et se relit par `showSaved()` et `LinkAnchors::read()` : une donnée nouvelle d'un composant
  ou d'un lien écrite là se copie d'elle-même ; une donnée de la scène écrite ailleurs (les zones) ne se copie pas.
  Les instances collées sont reliées aux JSON copiés par leur ordre : `Universe::insertInstances()` les rend dans
  celui du JSON, sans les stargates, comme `CopiedInstances::placed()`.
- Une étape défaite peut détruire et refaire tous les composants (`load()`) : ce qui garde un pointeur vers un
  composant de l'application le lâche sur `componentsChanged` ou par un `QPointer`, comme le `PropertyEditor`.

## Conventions

Elles sont décrites dans [CONVENTIONS.md](CONVENTIONS.md), à lire avant d'écrire du code et à tenir à jour quand une
règle change. L'essentiel :

- Base : le style de Qt (`camelCase`) et les C++ Core Guidelines.
- `.clang-format` : base LLVM, 4 espaces, 120 colonnes, accolades Allman, `if(` sans espace, `T* p`. `.clang-tidy`
  actif (bugprone, performance, readability, modernize…).
- Membres `m_camelCase`, `[[nodiscard]]` sur les accesseurs, `#pragma once`, `QStringLiteral` / `u"…"`.
- Découpage (hors tests) : une fonction fait une dizaine de lignes au plus (`readability-function-size` dans
  `.clang-tidy`, levé par `tests/.clang-tidy`) ; chaque étape nommable est extraite (espace de noms anonyme, méthode
  privée, ou en-tête partagé dès que deux classes en ont besoin), ce qui se répète est factorisé dès la deuxième fois.
  Une fonction trop longue qu'une PR modifie y est découpée, par un commit `refactor:` à part.
- Licence MPL-2.0 ([LICENSE](LICENSE)) : tout nouveau fichier source, en-tête, gabarit ou CMake commence par
  `// Copyright (C) <année> Fabien Di Grande` puis `// SPDX-License-Identifier: MPL-2.0` (`#` en CMake), et une
  ligne vide.
- Commentaires en anglais, au-dessus de la déclaration, qui disent le pourquoi ou le contrat.
- Commits en français, préfixés : `feat(NebulaStudio): …`, `fix(NebulaStudio): …`, `refactor: …`.
- Toute nouvelle fonctionnalité visible est décrite dans le README (sections Plugin, Component, NebulaStudio).

## Process

Chaque travail suit le cycle issue → branche → PR, sur `Shuko83/AppForge`, avec la CLI `gh`. Rien n'est commité ni
poussé directement sur `main`.

1. **Issue** : une par travail. Claude la rédige à partir de la demande (contexte, comportement attendu, critères
   d'acceptation) et la fait valider avant de la créer ; ou il part d'une issue existante.
2. **Branche** : depuis un `main` à jour (`git fetch`, puis fast-forward), nommée `feat/<n°>-<nom-court>`,
   `fix/<n°>-…`, `docs/<n°>-…` ou `refactor/<n°>-…`.
3. **Travail** : code, vérification, puis commits selon les conventions ci-dessus, qui citent l'issue (`#<n°>`) dans
   leur corps.
4. **PR** : push de la branche, puis PR vers `main` avec `Closes #<n°>`, un résumé, ce qui a été vérifié et ce qui ne
   l'a pas été. La CI compile chaque PR.
5. **Revue** : les corrections demandées vont sur la même branche.
6. **Fusion** : par l'utilisateur, en rebase. Ensuite, suppression de la branche et `main` local remis à jour.

Créer l'issue une fois validée, pousser la branche et ouvrir la PR font partie du cycle : Claude n'a pas à redemander.
Il ne fusionne jamais sans qu'on le lui demande.

Quand une demande sort du cycle, Claude le dit avant de continuer et propose d'y revenir : un changement demandé sans
issue, un commit ou un push sur `main`, une fusion sans PR, ou une demande sans rapport avec la branche en cours.

**Livraison d'une version** : le même cycle, puis un tag.

1. Une PR passe `project(Nebula VERSION x.y.z)` et les `find_package(Nebula x.y …)` du README à la nouvelle
   version, ajoute `docs/release-notes/<x.y.z>.md`, en anglais puis en français, et la section de la version en tête
   de `CHANGELOG.md`, avec son lien.
2. Une fois la PR fusionnée, le tag `<x.y.z>`, les trois nombres de la version sans préfixe `v`, est posé sur ce
   commit de `main` et poussé, à la demande de l'utilisateur. `release.yml` compile alors en Release, refuse un tag
   qui n'est pas la version du projet, installe le paquet, y ajoute les licences et le README, et publie la release
   GitHub avec le zip et ces notes.
3. Un tag ne se déplace pas. Si c'est le workflow du tag qui est en cause, il se corrige par une PR puis se relance à
   la main depuis `main` (`workflow_dispatch`, entrée `tag`), qui compile le commit du tag avec le fichier corrigé.
