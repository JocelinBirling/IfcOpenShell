Je veux pouvoir sauvegarder un ensemble de règle de filtrage.

Il me faut un éditeur de prédicat. 

https://namethatui.com/macos/rule-editor
Fonction de base: Exemples
Afficher les IfcWall et colorier en rouge les IfcSlab

On part du principe, qu'au début de chaque filtre, on part de rien. On donne en donnée d'entrée du filtre un liste d'élement. Pour le cas classique, cette liste contiendra l'ensemble des objets. Cependant, on pourra donner le contenu d'une box uniquement.



Sauvegarder les filtres
Un json.
On doit pouvoir se transmettre les règles.
Les filtres doivent être sauvegardé dans le cache de l'application pour survivre à la fermetture de la session.


Charger les filtres
On doit pouvoir sauvegarder les filtres de quelqu'un d'autres.


Arborescence

Dossier de Filtre

Dossier de Dossier

Ajouter un dossier

Ajouter un filtre



Créer un filtre

Lorsque l'on clique sur crée un filtre, une fenêtre temporaire va s'ouvrir.

On aura le nom. 
On aura la possibilité d'établir une description.
On aura la possibilité d'ajouter plusieurs couches.
Chaque couche sera décomposé en deux parties. Les actions seront lancés dans l'ordre de haut en bas.

Pour une couche, on retrouve;
	-La partie sélection, on réutilise les briques de l'IDS pour cela. On va avoir une spécification. Une spécification peut contenir plusieurs facettes de l'IDS. Une spécification doit permettre de sélectionner l'ensemble des éléments voulues.
	-La partie action

Si on a déjà crée une couche, on peut en créer une nouvelle et pour la partie sélection. 


Les actions
Une action est une modification de la maquette.

Isoler de la vue
Ajouter à la vue
Retirer de la vue
Colorier 
Changer la transparence
