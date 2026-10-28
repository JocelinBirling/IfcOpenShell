






Il faut créer un panneau BCF.

Je veux implémenter toutes les fonctionnalités du BCF que l'on trouve ici. 
https://github.com/BuildingSMART/BCF-XML/tree/release_3_0/Documentation


Dans les settings, on doit pouvoir renseigner les informations de l'author pour bien renseigner le BCF automatiquement.

On va aussi mettre en place DocumentReference, pour faciliter l'outverture d'un document, via un lien HTTP, ou bien via un lien windows. On doit pouvoir lire le lien.
Si le lien commence par HTTP, on lancera le navigateur par défaut.
Sinon, c'est un lien physique.
Si c'est un lien relatif, on le lancera par rapport à la position du BCF.
Si c'est un lien absolue, on ouvrira le fichier.




## Fonction du panneau

Le panneau BCF doit avoir les fonctions suivantes:
Load un fichier BCF
Save un fichier BCF

Load vient ouvrir l'exploreur pour trouver le fichier.

Save vient ouvrir l'explorateur afin de pouvoir renseigner la position du fichier, ainsi que la version du fichier.

Une barre des recherches doit être disponnible pour rechercher dans le nom des sujets. 
Par défaut, la barre de recherche ne vient rechercher que le nom du sujet. 
Comme la barre de recherche, -fs vient chercher dans l'ensemble des
propriétés du sujet BCF.

On pourra choisir le mode d'affichage des topics:
-Tableau avec 1 topic par ligne
-Miniature 
-Arbre avec les topics Related (On verra les topics liés de chaque topics)




## Fonction sur un sujet

Avec un clic droit sur un sujet, on peut:
Zoomer sur les objets
Isoler les objets
Modifier un sujet
Supprimer un sujet

Un double clic sur les objets permet de zoomer et d'isoler les objets.

Lorsque l'on clic sur modifier le sujet, cela ouvre une page qui vient présenter l'ensemble du sujet, avec les différents commentaires.

## Page de modification d'un sujet
On aura ici l'ensemble des données que l'on peut modifier.

Pour chaque commentaires, on voir l'image et le texte associé.

Un clic droit sur un commentaire doit permettre d'ouvrir les fonctions suivantes:
Isoler les objets
Zoomer sur les objets
Reprendre une image
Actualiser les données

Un double clique sur le commentaire, la caméra doit se déplacer au niveau du viewpoint, et appliquer la selection, les couleurs et la visibilité du sujet.

On pourra également voir la liste des élements (via une autre fenêtre)
de la selection
de la visibilité
des couleurs

On pourra modifier ces informations, en retirant, ajoutant des objets et en changeant la couleur.


### Related Topics
On aura deux tableaux:
Les topics qui sont liés par ce topic
Les topics qui lis ce topic 

On cliquant sur les topics, on pourra bouger de topic en topic.



## Component
Une fois que l'on crée un sujet BCF, au moment de sa création on vient récupérer l'ensemble des informations de la vue active du moment.



Modify image from BCF with paint
Cela doit permettre de modifier l'image du BCF avec paint pour mettre du texte ou des informations. L'image sera directement renseigné dans l'outil, sans passer par un enregistrement. En fermant paint, ou l'outil de modification, on doit directement récupérer le fichier.








