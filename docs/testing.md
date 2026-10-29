# Tester Nebula

Les tests de Nebula s'écrivent avec [Qt Test](https://doc.qt.io/qt-6/qtest-overview.html) (QTest) : un exécutable
par classe testée, enregistré auprès de CTest, que la CI lance sur chaque pull request. Ils s'écrivent autant que
possible pilotés par les données (*data-driven*) : une fonction de test, une table de cas.

## Lancer les tests

Les tests sont construits avec le reste du dépôt ; l'option CMake `NEBULA_BUILD_TESTS` (`ON` par défaut) permet de
s'en passer. Depuis le dossier de build :

```bash
cmake --build . --config Debug
ctest -C Debug --output-on-failure
```

- `ctest -R TestUniverse` ne lance que les tests dont le nom correspond.
- `ctest -V` affiche le journal de chaque test, même réussi.
- Un exécutable accepte les options de QTest : `TestUniverse.exe saveAndLoad` ne lance qu'une fonction,
  `TestUniverse.exe saveAndLoad:QDate` qu'un cas, `-functions` les liste. Lancé hors de CTest, il doit trouver
  `Core.dll` et le Qt du build dans `PATH` (voir plus bas).
- Avec Visual Studio, chaque test est un projet du dossier `tests` de la solution, débogable tel quel : son
  environnement de débogage (`VS_DEBUGGER_ENVIRONMENT`) contient déjà ce qu'il faut.

Un test échoué fait échouer la CI (étape **Test** de `cmake-win-msvc-2022_Qt6.8.yml`) : une PR ne se fusionne
qu'avec des tests verts.

## Organisation

| Dossier | Contenu |
| --- | --- |
| `tests/CMakeLists.txt` | `nebula_add_test()`, `nebula_add_test_plugin()`, la sortie des tests dans `<build>/tests/bin[/<config>]` |
| `tests/Components/` | Bibliothèque statique `TestComponents` : les composants écrits pour les tests |
| `tests/Plugins/` | Les plugins écrits pour les tests, chargés depuis leur fichier : `FirstPlugin`, `SecondPlugin`, `ThirdPlugin` |
| `tests/Core/` | Un test par classe de Core : `TestComponent`, `TestComponentFactory`, `TestPluginManager`, `TestUniverse`, `TestGalaxy` (une galaxy posée, ses interfaces, ses propriétés exposées, son édition et sa relecture compris), `TestStargate`, `TestSettingsFile` |
| `tests/NebulaStudio/` | Les classes de NebulaStudio qui se passent de sa fenêtre : `TestColors`, `TestTheme`, `TestPluginModel`, `TestComponentModel`, `TestEditionView`, `TestEditionScene` (la zone d'édition : tracé d'un lien dans chaque style, sélection et suppression d'un lien, liens d'une interface retirée), `TestPropertyEditor` (ses groupes, qui suivent les objets qu'une propriété ajoute ou détruit), `TestGalaxyModel` |
| `tests/exemple/` | Les composants d'un plugin d'exemple, chargés depuis son fichier comme le fait une application : `TestWidgetsPlugin` (la fenêtre principale, sa barre, les actions et le compteur, assemblés par leurs interfaces), le seul test qui dépend d'un exemple, celui qu'il teste |

Les tests et leurs plugins ne sont ni installés ni copiés dans le paquet.

### `nebula_add_test()`

```cmake
nebula_add_test(TestUniverse LINK_LIBRARIES TestComponents)
```

Crée l'exécutable `<NAME>` à partir de `<NAME>.cpp` (et de `SOURCES`), le lie à `Qt6::Test` et à `LINK_LIBRARIES`,
active AUTOMOC et AUTORCC, et l'enregistre auprès de CTest. Il reçoit :

- `NEBULA_TEST_PLUGINS_DIR` et `NEBULA_OTHER_TEST_PLUGINS_DIR`, les dossiers des plugins de test (voir plus bas),
  construits avant lui ;
- `NEBULA_VERSION`, la version du projet.

Il ne dépend d'aucun plugin d'exemple. `TestWidgetsPlugin` reçoit en plus, de `tests/exemple/CMakeLists.txt`,
`NEBULA_EXAMPLE_PLUGINS_DIR`, le dossier de `WidgetsPlugin`, construit avant lui.

CTest le lance avec, devant `PATH`, le dossier de Qt puis celui de `Core.dll`, la plateforme `offscreen` (aucune
fenêtre ne s'ouvre, en local comme en CI) et `QT_FORCE_STDERR_LOGGING=1`, sans quoi Windows n'envoie le journal de
QTest qu'au débogueur. Le Qt du build passe avant le dossier de `Core.dll` : celui que windeployqt y a copié pour
NebulaStudio n'a pas la plateforme `offscreen`.

Cette plateforme a ses limites : elle n'a pas de polices (un texte montré, celui d'un `QLabel` visible, avertit qu'elle
ne trouve pas leur dossier) et avertit qu'elle ne suit pas les contraintes de taille d'une fenêtre (`This plugin does
not support propagateSizeHints()`) quand une fenêtre montrée change la taille qu'elle demande, celle d'une
`QMainWindow` avec une barre d'outils. Un test évite le premier en vérifiant la visibilité sans montrer le texte
(`isVisibleTo()`), et attend le second par `QTest::ignoreMessage()`. Un test qui ne peut pas éviter de montrer du texte
(`TestEditionScene` : les noms des composants, ses menus) charge les polices une fois dans `initTestCase()`, avant
`QTest::failOnWarning()` (`QFontDatabase::families()`) : l'avertissement y est écrit sans faire échouer, et ne revient
plus. Un menu ouvert (`QMenu::exec()`) avertit enfin qu'elle ne sait ni le mettre au premier plan (`This plugin does not
support raise()`) ni lui donner le clavier (`This plugin does not support grabbing the keyboard`), deux messages
attendus par `QTest::ignoreMessage()` à chaque menu.

Une classe de NebulaStudio n'est pas dans une bibliothèque : son test recompile ses sources (`SOURCES`), avec
`nebulastudio.qrc` quand il lui faut les icônes ou la police.

### Les composants de test

`tests/Components/` contient des composants que les tests pilotent directement, sans passer par un fichier de plugin :
`registerTestComponents()` les enregistre dans `ComponentFactory::instance()` sous l'id de plugin `Test` (ids
`Test.Probe`, `Test.Source`…). Ils passent par `NEBULA_REGISTER_COMPONENT`, dans `TestComponents`, un
`Nebula::Plugin` sans fichier ni objet Qt, comme ceux d'un plugin dans sa classe générée, et le test les récupère par
`qobject_cast`.

| Composant | Rôle |
| --- | --- |
| `Probe` | Rend publics `provideInterface()`, `consumeInterface()`, `consumeInterfaceList()` (`consumeObjects()`, `consumedList()`), `removeProvidedInterface()` et `removeConsumedInterface()` (`removeProvided()`, `removeConsumed()`) et `addPropertiesForQObject()`, journalise ce que Core appelle (`events()`), refuse de démarrer si `refusesToStart` ; une propriété par type qu'un projet enregistre, plus `point` (que JSON ne tient pas) et `starts` (en lecture seule) |
| `Source` | Fournit `object` (`QObject`) pendant l'exécution, déclaré à la construction |
| `Sink` | Consomme `object` (obligatoire), `timer` (`QTimer`, facultatif) et la liste `objects` (`objects()`), déclarés à la construction |
| `Panel` | Ajoute les propriétés de trois `Options` : une sans nom, deux dans le groupe `page` |
| `Outlets` | Fournit autant d'`Options` (`objects()`) que son `count`, `object1`, `object2`…, chacune avec ses propriétés dans le groupe de son nom : régler `count` en ajoute ou retire les dernières, avec leurs liens ; `active`, bornée par `count` et déclarée après elle, mais avant elle dans l'ordre alphabétique d'un JSON |

Un composant ajouté ici doit être enregistré dans `TestComponents.cpp`, à côté des autres : c'est le seul fichier de
la bibliothèque statique dont l'éditeur de liens est sûr de garder l'initialisation statique.

### Les plugins de test

`tests/Plugins/` contient des plugins que les tests chargent depuis leur fichier, comme le fait une application :
`PluginManager`, les plugins d'un projet ou d'une galaxy, leur déchargement et `PluginModel` se testent sur eux, pas sur
les plugins d'exemple, qui changent avec ce qu'ils montrent. `nebula_add_test_plugin()` les construit par
`nebula_add_plugin()`, comme un projet tiers construit les siens, dans un dossier à côté des tests mais pas dans leur
`plugins/` : chaque `Universe` scanne celui-là à sa construction, un test scanne lui-même ceux-ci. Il les retire du
paquet, où CMakeUtils met toute cible qu'il crée.

```cmake
nebula_add_test_plugin(
    NAME SecondPlugin
    DIRECTORY "${NEBULA_OTHER_TEST_PLUGINS_DIR}"
    DESCRIPTION "Second plugin of the tests, in another directory"
    VERSION 1.1.0
)
```

| Plugin | Dossier | Version | Composants |
| --- | --- | --- | --- |
| `FirstPlugin` | `NEBULA_TEST_PLUGINS_DIR` (`test-plugins/`) | 1.0.0 | `Alpha` |
| `SecondPlugin` | `NEBULA_OTHER_TEST_PLUGINS_DIR` (`other-test-plugins/`) | 1.1.0 | `Beta` |
| `ThirdPlugin` | `NEBULA_TEST_PLUGINS_DIR` | 2.0.0 | `Gamma`, `Delta` |

Leurs ids, `FabienDG.Nebula.FirstPlugin`…, se suivent dans l'ordre de leurs noms, le second seul dans l'autre dossier :
une liste triée par id range ce qu'elle y trouve entre les deux autres. Leurs composants ne font rien et se passent
d'un `QApplication`. Un plugin ajouté a son dossier dans `tests/Plugins/`, sa classe et ses composants dans `src/`
comme ceux d'un plugin d'exemple, et son `add_subdirectory()` dans `tests/Plugins/CMakeLists.txt` ; chaque test est
construit après lui.

## Écrire un test

Un fichier `Test<Classe>.cpp` par classe testée, dans le dossier de son module, ajouté par une ligne
`nebula_add_test()` dans le `CMakeLists.txt` de ce dossier. Il suit [CONVENTIONS.md](../CONVENTIONS.md), et en plus :

- La classe de test s'appelle comme le fichier, n'a pas d'en-tête, et se termine par `QTEST_MAIN(…)`
  (`QTEST_GUILESS_MAIN` pour Core, `QTEST_APPLESS_MAIN` sans application du tout) puis `#include "<Fichier>.moc"`.
- Un commentaire au-dessus de la classe dit ce qu'elle teste ; les noms des fonctions disent le comportement attendu :
  `startRefused()`, `destroyedProviderUnbinds()`.
- `init()` appelle `QTest::failOnWarning()` : un avertissement que le test n'attend pas le fait échouer. Celui qu'il
  attend est annoncé avant par `QTest::ignoreMessage()`, qui échoue à son tour s'il ne vient pas.
- Ce qui n'existe que pour le test (fichiers, dossiers) va dans un `QTemporaryDir`. Un test qui écrit dans les dossiers
  standard appelle d'abord `QStandardPaths::setTestModeEnabled(true)`.

### Piloté par les données

Dès qu'un même comportement se vérifie sur plusieurs entrées, la fonction de test a sa table, `<fonction>_data()` :
une colonne par entrée et par résultat attendu, une ligne nommée par cas.

```cpp
void TestUniverse::rename_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("renamed");

    QTest::newRow("new name") << "Renamed" << true;
    QTest::newRow("its own name") << "Probe1" << true;
    QTest::newRow("name of another") << "Probe2" << false;
    QTest::newRow("empty") << "" << false;
}

void TestUniverse::rename()
{
    QFETCH(QString, name);
    QFETCH(bool, renamed);

    Universe universe;
    Probe* first = ::instantiate<Probe>(universe);
    ::instantiate<Probe>(universe);

    QCOMPARE(universe.rename(*first, name), renamed);
}
```

- Le nom d'une ligne dit le cas, pas la valeur : `"name of another"`, pas `"Probe2"`. Il apparaît dans le journal
  d'un échec : `FAIL! : TestUniverse::rename(name of another)`.
- Une ligne par cas limite, y compris ceux qui échouent : nom vide, id inconnu, type différent.
- Quand une colonne devrait contenir un objet que la ligne ne peut pas créer (un composant, une entrée), elle contient
  une clé, que la fonction de test traduit (`QHash<QString, …>`).
- Un type sans métatype de Qt ne peut pas être une colonne ; une énumération de Nebula le peut (`ComponentState`,
  `GlossaryTerm`).

### Vérifier

- `QCOMPARE` plutôt que `QVERIFY(a == b)` : son échec montre les deux valeurs. `QVERIFY2(condition, message)` quand
  il n'y a pas de valeur à comparer mais quelque chose à dire, comme `errorString()`.
- `QSignalSpy` compte les signaux ; `QTRY_COMPARE` attend ce qui arrive par la boucle d'événements (l'écriture
  regroupée d'un fichier de réglages).
- `QAbstractItemModelTester` surveille un modèle de NebulaStudio pendant tout le test.
- Une vue se pilote en envoyant des `QMouseEvent` à son `viewport()` (`TestEditionView`). Un déplacement sans bouton
  n'y arrive que si la vue suit la souris, ce que fait une `QGraphicsView` dès qu'un item de sa scène accepte le
  survol ou a un curseur, comme ceux de NebulaStudio. Un clic qu'aucun item ne prend s'envoie de même, plutôt que par
  `QTest::mouseClick()`, qui avertit que la vue n'a pas accepté le relâchement.
- Un menu contextuel bloque dans `exec()` : le test programme d'abord, par `QTimer::singleShot(0, …)`, ce qui le lit
  (`QApplication::activePopupWidget()`) puis le ferme, par Entrée sur l'action choisie (`setActiveAction()`) ou par
  Échap, puis envoie le `QContextMenuEvent` au `viewport()` (`TestEditionScene::contextMenu()`).
- Un test qui passe du premier coup se vérifie en cassant exprès le code qu'il couvre : il doit échouer.
