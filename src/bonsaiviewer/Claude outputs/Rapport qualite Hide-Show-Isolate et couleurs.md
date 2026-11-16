<!-- This file was generated with the assistance of an AI coding tool. -->

# Rapport qualité — Hide / Show / Isolate et gestion des couleurs

Date : 2026-11-13 · Branche : `BonsaiViewerPlus` (commit `021fbc96a0`)
Priorités demandées : **performance**, **bugs / cohérence**, **mémoire**.
Périmètre : `src/bonsaiviewer` **et** `src/ifcviewer` (ViewportCore).
Pas de symptôme mesuré : les coûts sont estimés à partir du code (N = nombre d'objets de la fédération, I = nombre d'instances GPU, k = taille de l'ensemble traité, G = nombre de couleurs distinctes).

---

## 1. Synthèse

L'architecture est saine sur le papier : un seul point d'entrée pour la visibilité (`ActiveView`, B02), un seul pour les couleurs (`AppearanceService`, B04, avec ses 7 couches de priorité), un exécuteur partagé (`ActionPipeline`, B08) utilisé par le ruban, les filtres, la Box, le BCF. Le code est commenté et lisible.

Les problèmes sont concentrés à **la frontière service → viewport** :

1. **Chaque appel couleur parcourt toutes les instances de tous les modèles et ré-uploade tout le buffer d'instances** du modèle touché. `AppearanceService` découpe ensuite une opération en *un appel par couleur*, et certains appelants (tableau, auto-couleur) en *un appel par groupe ou par ligne*. Coût réel : **O(G × I)** plus G uploads GPU complets, alors qu'on attendrait O(k).
2. **La visibilité est stockée comme l'ensemble des objets cachés** (`unordered_set`). Un Isolate de 10 objets sur 500 000 insère ~500 000 entrées, et le cull fait ensuite une recherche de hash par instance.
3. **Le registre d'éléments est copié en entier** (`basicElementInfoForModel` renvoie un `vector<BasicElementInfo>` par valeur, avec 3 `QString` par élément) dans tous les chemins chauds : isolate, visibleIds, hiddenIds, filtre de catégorie, undo.
4. Quelques **chemins contournent les services** (arbre spatial) et cassent les invariants (catégories désactivées, undo).

Les 5 corrections du tableau ci-dessous apportent l'essentiel du gain.

| # | Sujet | Gain estimé | Effort | Où |
|---|-------|-------------|--------|----|
| P1 | Couleur en lot : un seul passage par appel `applyToViewport` | ×G sur auto-couleur / tableau / IDS / diff | Moyen | ifcviewer + AppearanceService |
| P2 | Tableau coloré : un appel par ligne → un appel global | ×(nb lignes) | Faible | `modules/table/View.cpp` |
| P3 | Registre : itérer sans copier | Supprime N copies de `QString` par action | Faible | `ElementRegistry` |
| P4 | Index par catégorie dans `CategoryOverrideFilter` | O(N × nb catégories) → O(k) par mutation | Faible | `CategoryOverrideFilter` |
| P5 | Visibilité en bitset dense par `object_id` | Isolate O(N) sans hash, cull plus rapide | Moyen | `VisibilityState` (ifcviewer) |

---

## 2. Performance

### P1 — `setObjectsColor` : un scan complet + un upload complet par couleur (critique)

**Constat**

- [ViewportCore.cpp:6597](../../ifcviewer/ViewportCore.cpp:6597) `setObjectsColor` construit un `unordered_set` des ids, puis parcourt **toutes les instances de tous les modèles**, et appelle `uploadInstanceRecords(m)` si le modèle est touché.
- [ViewportCore.cpp:361](../../ifcviewer/ViewportCore.cpp:361) `uploadInstanceRecords` reconstruit le miroir de cull complet (`rebuildCullInstances`, O(I)), recompose un `vector<InstanceGpu>` complet et l'envoie au GPU en entier.
- [AppearanceService.cpp:178](services/AppearanceService.cpp:178) `applyToViewport` regroupe par couleur, puis fait **un appel viewport par couleur distincte** (plus un pour `restore`, plus un par niveau d'alpha).
- [ActionPipeline.cpp:130](services/ActionPipeline.cpp:130) `autoColourByValues` appelle `setLayerColour` **une fois par valeur distincte**, donc `applyToViewport` une fois par groupe.

**Coût** : auto-couleur sur 200 valeurs × 500 000 instances = ~100 M recherches de hash + 200 reconstructions du miroir de cull + 200 uploads complets du buffer d'instances. Les mêmes multiplicateurs touchent IDS (2 appels), Diff (4 couleurs + 2 opacités), Clash (3), BCF.

**Recommandation** (par ordre de gain) :

1. **API en lot dans ViewportCore** : `setObjectsColors(const std::vector<std::pair<uint32_t, uint32_t>>& id_rgba, flags)` ou mieux une table `object_id → (rgba8, flags)`. Un seul passage sur les instances, un seul upload par modèle. `AppearanceService::applyToViewport` n'appelle plus qu'une fois.
2. **Index `object_id → indices d'instance`** par modèle (les `object_id` sont denses : `object_id_base` + compteur local, voir `ModelGpuData.h:344`). La mise à jour devient O(k) au lieu de O(I), et l'upload peut se limiter aux plages modifiées (`wgpuQueueWriteBuffer` avec offset).
3. **Option structurelle** : un buffer GPU de couleur **par objet**, sur le modèle exact du buffer de flags de sélection déjà existant (`ensureSelectionFlagsBuffer` / `uploadSelectionFlagsIfDirty`, [ViewportCore.h:404](../../ifcviewer/ViewportCore.h:404)) : tableau indexé par `object_id`, flag « dirty », upload une fois par frame. Le changement de couleur devient O(k) côté CPU et un seul upload par frame. Point d'attention : le cull lit l'alpha de `cull_instances[i].color_override_rgba8` pour séparer opaque / transparent ; il faudrait lire ce tableau par objet à la place.
4. `ActionPipeline::autoColourByValues` : construire toutes les paires (id, couleur) puis **un seul** appel `AppearanceService`. Il faut pour cela une méthode `setLayerColours(layer, vector<pair<id, QColor>>)`.

### P2 — Tableau : un appel couleur par ligne (critique sur grands tableaux)

[modules/table/View.cpp:465](modules/table/View.cpp:465) appelle `setLayerColour` **dans la boucle sur les lignes**. Un tableau de 2 000 lignes déclenche 2 000 scans complets + 2 000 uploads. À regrouper en un seul appel (dépend de la méthode en lot de P1.4 ; même sans P1.1, ça ramène à un seul `applyToViewport`, qui groupe déjà par couleur).

### P3 — Copies du registre d'éléments dans tous les chemins chauds

- [ElementRegistry.cpp:54](ElementRegistry.cpp:54) `basicElementInfoForModel` renvoie un `vector<BasicElementInfo>` **par valeur** (3 `QString` + ids par élément).
- [ElementRegistry.cpp:75](ElementRegistry.cpp:75) `findBasicElementInfo` renvoie un `optional<BasicElementInfo>` **par valeur**.

Appelants concernés : `ActiveView::allObjectIds` (isolate, visibleIds, hiddenIds, restoreHidden), `CategoryOverrideFilter::objectIdsForCategory` (une copie complète **par catégorie désactivée et par mutation**), `CategoryOverrideFilter::isObjectAllowed` (une copie par objet dans `visibleIds()` → N copies pour « Select Everything »), `AppearanceService::purgeModel`, `FilterRunner::startTarget`, arbre spatial.

**Recommandation** : exposer `const std::vector<uint32_t>& objectIdsForModel(id)` (le vecteur `ids_by_model_` existe déjà) et `const BasicElementInfo* find(id)` (pointeur, pas de copie). Changement mécanique, gain immédiat partout.

### P4 — Filtre de catégorie ré-appliqué par scan complet après chaque mutation

[CategoryOverrideFilter.cpp:85](services/CategoryOverrideFilter.cpp:85) : après **chaque** `isolate / add / showAll / invert`, pour **chaque** catégorie désactivée, copie du registre + comparaison de `QString` sur tous les éléments. Avec 3 catégories désactivées (IfcSpace, IfcOpeningElement, IfcZone : le cas typique), c'est 3 × N comparaisons de chaînes par clic.

**Recommandation** : un cache `QHash<QString, std::vector<uint32_t>>` des ids par classe, invalidé sur `modelGeometryReady` / `modelsChanged`. Mieux : dans `add(ids)`, ne filtrer que `ids` (on ne peut pas ramener un objet qu'on n'a pas touché) au lieu de recacher toute la catégorie.

### P5 — Visibilité stockée comme « ensemble des cachés » (hash set)

- [VisibilityState.h](../../ifcviewer/VisibilityState.h) : `std::unordered_set<uint32_t> hidden_ids_`.
- [ActiveView.cpp:38](services/ActiveView.cpp:38) `isolate` : construit un `unordered_set` des ids voulus, puis le complément (≈ N), puis N insertions dans le hash set du viewport.
- [ViewportCore.cpp:3231](../../ifcviewer/ViewportCore.cpp:3231) : le cull fait `visibility_.isHidden()` (recherche de hash) **pour chaque instance**, à chaque cull où la caméra ou l'époque change.

**Recommandation** : remplacer le hash set par un `std::vector<uint8_t>` (ou bitset) indexé par `object_id` (ids denses, déjà utilisés ainsi par les flags de sélection). `isHidden` devient un accès tableau, `hideAll` un `fill`, `isolate` un `fill(1)` + k remises à 0, `invert` un parcours linéaire. Garder `hiddenCount()` via un compteur. Bonus : le cull devient plus rapide en navigation dès qu'un Isolate est actif.

### P6 — Filtres : isolations successives et passes complètes

- [FilterRunner.cpp:330](services/FilterRunner.cpp:330) vide la vue avec `run({}, Isolate)`, ce qui calcule un complément de taille N. `hideAll()` suffit et évite l'allocation.
- [FilterRunner.cpp:343](services/FilterRunner.cpp:343) chaque action Isolate recopie `kept_ids_`, trie, déduplique, puis ré-isole toute la fédération : O(M × N) pour M modèles. Puisque la vue a été vidée en début de run, un `add(object_ids)` donnerait le même résultat pour tous les modèles sauf si une action précédente du même run a ajouté des objets à retirer. À valider avec la sémantique TI06 avant de changer.
- Chaque action de vue émet `visibilityChanged` → `ViewportView::refresh` → boucle sur tous les modèles (coordonnées, transformation, visibilité modèle). C'est gardé par des comparaisons de matrices donc peu coûteux, mais cela mélange deux responsabilités (voir B4).
- [ActiveFilters.cpp:64](services/ActiveFilters.cpp:64) : le rechargement d'**un** modèle relance **tous** les filtres activés sur **tous** les modèles (sélection IfcTester complète). On pourrait limiter la passe forcée au modèle rechargé (les couleurs des autres modèles n'ont pas été purgées).
- Modifier un filtre activé relance tous les filtres activés (`clearForced` global). Un cache des résultats de sélection par filtre (clé : JSON du document + modèle) éviterait les sélections IfcTester inchangées.

### P7 — Coût caché de l'undo à chaque clic

[UndoStack.cpp:34](services/UndoStack.cpp:34) `record()` est appelé **avant chaque** mutation de vue ou de couleur et copie :
- `ActiveView::hiddenIds()` : copie du registre + N recherches, et un vecteur de taille « nombre de cachés » (≈ N après un Isolate) ;
- `AppearanceService::snapshot()` : copie complète de `states_`, soit 7 × (bool + uint32) + opacité ≈ 40 octets par objet coloré.

Avec `kCapacity = 10`, après quelques Isolate + auto-couleur sur 500 000 objets : ~10 × (2 Mo + 20 Mo) gardés en mémoire, et le même volume recopié à chaque clic.

**Recommandation** : avec le bitset de P5, l'état caché tient en N/8 octets (≈ 60 Ko pour 500 000 objets). Pour les couleurs, stocker un diff (les ids touchés et leur ancien état) plutôt qu'un snapshot complet ; `setColour/setOpacity` connaissent déjà les ids touchés.

---

## 3. Bugs et cohérence

### B1 — L'arbre spatial contourne `ActiveView` (bug)

[modules/spatial_hierarchy/View.cpp:214](modules/spatial_hierarchy/View.cpp:214) appelle directement `viewport_->setObjectsVisible(...)`. Conséquences :
- **une catégorie désactivée peut revenir** en cochant un nœud (IfcSpace contenu dans un étage), ce qui viole l'invariant B03 (« a disabled category is never shown, whatever the path ») ;
- **pas d'undo** enregistré ;
- **pas de `notifyVisibilityChanged`**, donc les autres vues ne sont pas prévenues.

Correction : passer par `active_view_->add / remove`.

### B2 — Undo et filtres activés désynchronisés (bug latent)

[AppearanceService.cpp:137](services/AppearanceService.cpp:137) `restore(snapshot)` remplace `states_`, **couche forcée comprise**, mais ne touche pas `forced_opacity_`. Scénario : on peint → un filtre activé change (passe forcée : `clearForced` + ré-application) → Undo. Le snapshot ré-injecte les anciennes couleurs forcées et efface les nouvelles, tandis que `forced_opacity_` reflète l'état actuel. Résultat : l'affichage ne correspond plus aux filtres activés jusqu'à la prochaine passe.

Correction : `restore` doit conserver la couche `ForcedFilter` et l'opacité forcée actuelles et ne restaurer que les autres couches (la couche forcée n'est de toute façon pas dans le périmètre de l'undo, cf. `FilterRunner` qui suspend l'undo).

### B3 — Plusieurs points d'undo pour une seule action

- Diff : [modules/diff/View.cpp:376-377](modules/diff/View.cpp:376) et 383-384, 391-392 : deux `setOpacity` consécutifs créent deux points d'undo. L'utilisateur doit faire Undo deux fois.
- Restore View ([MainWindow.cpp](MainWindow.cpp), bouton « Restore View ») : `clearOverrides()` ne fait pas de `record()`, `showAll()` en fait un. Un Undo après Restore View rend la visibilité mais **pas les couleurs**.
- `clearLayer` ne fait jamais de `record()` (IDS, tableau, diff, clash) : cohérent si c'est voulu, mais non documenté.

Correction : un garde RAII `UndoStack::Group` (un seul `record()` au début, suspendu ensuite) autour des actions composées.

### B4 — `invertElementVisibility` ne marque pas le cull comme modifié (bug masqué)

[ViewportWindow.cpp:1156](../../ifcviewer/ViewportWindow.cpp:1156) modifie `visibility_` **sans** `markCullInputsChanged()`, contrairement à toutes les autres opérations (déplacées dans `ViewportCore`). Le cull est sauté si l'époque n'a pas changé ([ViewportCore.cpp:7975](../../ifcviewer/ViewportCore.cpp:7975)). Ça fonctionne aujourd'hui uniquement parce que `ActiveView::invert` émet `visibilityChanged` → `ViewportView::refresh` → `showModel/hideModel`, qui incrémente l'époque **avant** leur early-return. Le jour où `refresh` ne sera plus branché sur `visibilityChanged`, Invert View ne s'affichera plus avant un mouvement de caméra.

Correction : déplacer `invert` dans `ViewportCore` avec `markCullInputsChanged()`, comme les autres verbes.

### B5 — Sémantique d'Invert sur les modèles cachés

Le même `invertElementVisibility` saute les modèles cachés, puis fait `visibility_.clear()`. Les éléments cachés individuellement dans un modèle masqué perdent leur état : quand on ré-affiche le modèle, tout réapparaît. À trancher fonctionnellement.

### B6 — Entrées mortes dans `AppearanceService::states_`

`removeColour` ([AppearanceService.cpp:74](services/AppearanceService.cpp:74)) et `clearLayer` laissent des entrées sans aucune couche et à opacité 255. Elles ne sont jamais supprimées (sauf `clearOverrides` / rechargement), grossissent chaque snapshot d'undo et sont ré-appliquées inutilement par `restore`. Correction : effacer l'entrée quand elle redevient neutre (et pas d'opacité forcée).

### B7 — Verbes dupliqués dans le viewport

`ViewportCore::hideSelected / isolateSelected` (basés sur la sélection du viewport) coexistent avec `ActiveView` (basé sur `SelectionService`). Ils ne sont plus appelés par BonsaiViewer (seulement exposés par `ViewportWindow`). S'ils restent utilisés ailleurs (web), les documenter comme tels ; sinon, ils sont une seconde voie qui contourne B02/B03.

---

## 4. Mémoire

| Structure | Taille actuelle | Piste |
|-----------|-----------------|-------|
| `VisibilityState::hidden_ids_` (hash set) | ~32–40 o/objet caché (≈ 16–20 Mo après Isolate sur 500 k) | bitset : 1 bit/objet (≈ 60 Ko) — P5 |
| `UndoStack` (10 états) | 10 × (vecteur caché + copie de `states_`) | bitset + diff de couleurs — P7 |
| `AppearanceService::ObjectState` | 7 bool + 7 uint32 + 1 octet ≈ 40 o/objet | stocker seulement les couches actives (masque 8 bits + petit tableau), ou un `uint32` par couche dans des maps séparées par couche |
| Copies du registre | temporaire, N × (3 `QString` + ints) par appel | références / pointeurs — P3 |
| Entrées neutres dans `states_` | croît sans limite pendant la session | B6 |

---

## 5. Points positifs à conserver

- Un seul exécuteur (`ActionPipeline`) pour le ruban, les filtres, la Box et le BCF : les optimisations de P1/P5 profiteront à tous les chemins d'un coup.
- Couches de priorité explicites dans `AppearanceService` : modèle clair, facile à tester.
- `applyToViewport` regroupe déjà par couleur, et `setObjectsColor` saute déjà les instances inchangées : la bonne intention est là, il manque l'API en lot côté viewport.
- `FilterRunner` : construction de la table EXPRESS id → object id une fois par modèle, `set_union` sur des vecteurs triés, `unordered_set` pour RemoveFromView (le commentaire montre qu'un quadratique a déjà été corrigé).
- `ActiveFilters::schedule` regroupe les signaux d'un même événement (`singleShot(0)`).

---

## 6. Tests

Aucun test n'existe pour `ActiveView`, `AppearanceService`, `CategoryOverrideFilter` ni `UndoStack` (le dossier `tests/` ne couvre que le clash, les arbres et l'index). Avant d'optimiser, je recommande des tests unitaires Qt sur :
- la priorité des couches et le retour à la couleur d'origine (`removeColour` garde l'opacité) ;
- `clearOverrides` qui conserve la couche forcée ;
- l'undo : restauration de la visibilité et des couleurs, et B2 (filtres activés) ;
- l'invariant B03 : aucune voie (isolate, add, showAll, invert, arbre spatial) ne ramène une catégorie désactivée.

Ces services dépendent de `ViewportWindow` concret : introduire une petite interface (`setObjectsVisible`, `isObjectHidden`, `setObjectsColor`, `clearObjectColors`) permettrait de les tester sans GPU.

---

## 7. Ordre de travail proposé

1. **Tests** des services (section 6) : filet de sécurité.
2. **B1** (arbre spatial) et **B4** (invert) : petits correctifs, risque faible.
3. **P3** (registre sans copie) et **P2** (tableau en un appel) : mécaniques, gain immédiat.
4. **P1** couleur en lot dans ViewportCore (étape 1 puis 2 : API en lot + index object_id → instances).
5. **P5** bitset de visibilité, puis **P7** undo allégé (qui en dépend).
6. **P4**, **B2**, **B3**, **B6**, puis P6 (filtres) à valider fonctionnellement.

Chaque étape est un commit indépendant, conformément à AGENTS.md.
