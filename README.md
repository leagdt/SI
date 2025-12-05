# Projet de synthèse d'images 3D

Ce dossier regroupe deux TP basés sur gkit2light. Le premier porte sur le rendu indirect, deferred rendering, et le second sur le lancer de rayons.

## Rendu Indirect

Ce TP consiste à créer une application graphique complète qui charge une scène 3D ainsi que ses textures, puis l’affiche en gérant une caméra mobile.
Des sources de lumière ponctuelles sont ajoutées, et des calculs d’éclairage sont effectués pour chaque pixel. Pour optimiser ces calculs, on met en place un pipeline de rendu différé (G-Buffer), puis on améliore encore les performances grâce au frustum culling.

### Structure du Projet

- **projets/maison.cpp** : Implémente la classe `Skeleton` pour gérer les articulations du squelette et leurs transformations.
- **shader/textureMaison.glsl** : Gère l'affichage et la mise à jour du squelette.
- **shader/textureHeightMap.glsl** : Contrôle le déplacement du personnage via le clavier.
- **shader/program_deffered.glsl** : Gère la physique des particules et leur interaction avec le personnage.
- **shader/gbuffer.glsl** : Gère la physique des particules et leur interaction avec le personnage.

### Fonctionnalités

#### Partie 1 : Affichage de la scène et gestion de la caméra

- Implémentation de la classe `Skeleton` pour gérer un tableau d'articulations (`SkeletonJoint`).
- Affichage des squelettes en utilisant les transformations appropriées.
- Initialisation et mise à jour des positions des articulations.

#### Partie 2 : Placement des lumières et calcul de la couleur

- Implémentation de la classe `CharacterController` pour contrôler le déplacement du personnage.
- Déplacement d'une sphère en utilisant les touches du clavier pour accélérer, freiner, tourner et sauter.

#### Partie 3 : Rendu indirect

- Calcul de distance entre 2 poses afin de construire le graphe d'animation.
- Construction automatique d'un graphe d'animation pour gérer les transitions entre animations (4 animations :Idle, Walk, Run et Kick).
- Implémentation d'une machine à état (classe `FiniteStateMachine`), qui nous permet de passer dans différent états, comme Idle, walk, Jump, Backflip...
- Gestion du temps et interpolation entre 2 frames. 

#### Partie 4 : Frustum Culling

- Implémentation de la physique des particules.
- Gestion des collisions entre les particules et le sol. (Effet de balle qui rebondit)
- Le personnage peut intéragir avec les particules s'il y a collision. 

### Instructions de Compilation et d'Exécution

#### Compilation

Pour compiler le programme, ouvrez un terminal dans le répertoire du projet et exécutez les commandes suivantes :

```sh
premake4 gmake
make maison
```

#### Exécution

Pour exécutez le programme, dans le même terminal, exécutez une des commandes suivantes : 

- Si vous voulez lancer le mode avec le graphe d'animation automatique : 

```sh
./bin/maison
```

## Lancer de rayons

Le TP consiste à générer une image en lançant des rayons depuis la caméra vers une scène 3D. La méthode de Monte Carlo est ensuite utilisée pour simuler l'éclairage global. Enfin, pour optimiser le rendu, l’estimation de Monte Carlo est améliorée en ciblant directement les sources lumineuses.

### Structure du Projet

- **projets/lancerRayons.cpp** : Implémente la classe `Skeleton` pour gérer les articulations du squelette et leurs transformations.

### Fonctionnalités

#### Partie 1 : Rendu de l'image en utilisant le lancer de rayons


#### Partie 2 : Calcul de l'éclairage 


#### Partie 3 : Eclairage direct efficace

### Instructions de Compilation et d'Exécution

#### Compilation

Pour compiler le programme, ouvrez un terminal dans le répertoire du projet et exécutez les commandes suivantes :

```sh
premake4 gmake
make lancerRayons
```

#### Exécution

Pour exécutez le programme, dans le même terminal, exécutez une des commandes suivantes : 

- Si vous voulez lancer le mode avec le graphe d'animation automatique : 

```sh
./bin/lancerRayons
```
