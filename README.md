# Projet de synthèse d'images 3D

Ce dossier regroupe deux TP basés sur gkit2light. Le premier porte sur le rendu indirect, deferred rendering, et le second sur le lancer de rayons.

## Rendu Indirect

Ce TP consiste à créer une application graphique complète qui charge une scène 3D ainsi que ses textures, puis l’affiche en gérant une caméra mobile.
Des sources de lumière ponctuelles sont ajoutées, et des calculs d’éclairage sont effectués pour chaque pixel. Pour optimiser ces calculs, on met en place un pipeline de rendu différé (G-Buffer), puis on améliore encore les performances grâce au frustum culling.

### Structure du Projet

- **projets/maison.cpp** : Crée et exécute une application 3D, préparant les objets, textures et buffers pour les envoyer aux shaders et effectuer le rendu.
- **shader/textureMaison.glsl** : Ce shader permet de réaliser un rendu direct de la scène. Il transforme les sommets et transmet les informations nécessaires au calcul de la lumière. Il applique la texture, gère les fragments transparents et calcule la couleur finale à partir de plusieurs sources lumineuses.
- **shader/textureHeightMap.glsl** : Ce shader permet de générer la heightmap de la scène. Il calcule et renvoie une hauteur pour chaque pixel.
- **shader/gbuffer.glsl** : Ce shader remplit un g-buffer en renvoyant dans différentes textures les informations nécessaires : la couleur de la matière, la normale et la profondeur.
- **shader/program_deffered.glsl** : Ce shader est utilisé pour le rendu indirect. Il récupère les informations du g-buffer et calcule la couleur finale en tenant compte des différentes sources de lumière.

### Fonctionnalités

#### Partie 1 : Affichage de la scène et gestion de la caméra

- Gestion de la caméra : appuyer sur "O" pour changer de point de vue (mode orbiter ou première personne).
- Déplacements : en mode première personne, les touches Z, Q, S, D permettent de se déplacer. Une heightmap est utilisée pour éviter de traverser les murs et permettre de monter sur de petits obstacles. Cependant, cette méthode pose un problème avec les toits, car seule une hauteur est stockée par position. Il faudrait lancer un rayon pour corriger cela, mais ce serait plus coûteux.
- Gestion de la transparence des objets.

#### Partie 2 : Placement des lumières et calcul de la couleur

- Calcul de l'éclairage direct : trop lent quand le nombre de lumière est trop important.

#### Partie 3 : Rendu indirect

- Construction du G-Buffer : stockage des informations essentielles de la scène comme la couleur, les normales et la profondeur en plusieurs textures.
- Calcul de l’éclairage indirect : utilisation des données du G-Buffer pour déterminer la couleur finale des pixels en fonction des différentes sources de lumière.

#### Partie 4 : Frustum Culling

la scène 3D est découpée en cubes NxNxN. Pour chaque cube, trois tests sont effectués pour déterminer s’il est entièrement hors du frustum :
- Test dans le repère monde : on vérifie si tous les sommets du cube se trouvent d’un seul côté d’un plan du frustum.
- Test dans le repère projectif : si le cube n’a pas été exclu, on teste à nouveau la position des sommets par rapport aux plans du frustum transformés.
- Test des sommets individuels : si le cube semble partiellement à l’extérieur, on vérifie si au moins un sommet est complètement hors du frustum ; si tous les sommets sont hors, le cube n’est pas dessiné, ce qui optimise le rendu.

### Instructions de Compilation et d'Exécution

#### Compilation

Pour compiler le programme, ouvrez un terminal dans le répertoire du projet et exécutez les commandes suivantes :

```sh
premake4 gmake
make maison
```

#### Exécution

Pour exécutez le programme, dans le même terminal, exécutez une des commandes suivantes : 

```sh
./bin/maison
```

## Lancer de rayons

Le TP consiste à générer une image en lançant des rayons depuis la caméra vers une scène 3D. La méthode de Monte Carlo est ensuite utilisée pour simuler l'éclairage global. Enfin, pour optimiser le rendu, l’estimation de Monte Carlo est améliorée en ciblant directement les sources lumineuses.

### Structure du Projet

- **projets/lancerRayons.cpp** : Génère une image qui permet de visualiser une scène 3D grâce à du lancer de rayons.

### Fonctionnalités implémentées 

#### Partie 1 : Rendu de l'image en utilisant le lancer de rayons
Cette partie consiste à générer l’image en utilisant la technique du lancer de rayons (ray tracing), qui simule le trajet des rayons lumineux depuis la caméra jusqu’aux objets de la scène.
#### Partie 2 : Calcul de l'éclairage 
Dans cette partie, une méthode de Monte Carlo simple est implémentée afin de générer aléatoirement la direction des rayons. Cette approche permet d’approximer l’éclairage global de manière statistique.
#### Partie 3 : Eclairage direct efficace
Dans cette partie, la méthode de Monte Carlo est appliquée uniquement aux sources de lumière afin d’accélérer le rendu. Cette optimisation permet d’obtenir une image plus propre, avec moins de bruit, tout en réduisant le nombre de rayons nécessaires.

### Instructions de Compilation et d'Exécution

#### Compilation

Pour compiler le programme, ouvrez un terminal dans le répertoire du projet et exécutez les commandes suivantes :

```sh
premake4 gmake
make lancerRayons
```

#### Exécution

Pour exécutez le programme, dans le même terminal, exécutez une des commandes suivantes : 

-Cette commande lance le programme avec les paramètres par défaut : 16 rayons par pixel pour simuler la lumière et l’utilisation de la méthode de Monte Carlo efficace (les rayons sont dirigés directement vers les sources de lumière) :

```sh
./bin/lancerRayons
```

- Cette commande lance le programme avec 256 rayons par pixel et la méthode de Monte Carlo efficace :

```sh
./bin/lancerRayons 256
```

- Cette commande lance le programme avec 256 rayons par pixel et la méthode de Monte Carlo simple :

```sh
./bin/lancerRayons 256 1
```


#### Résultat

Une image est générée à la fin de l’exécution du programme. Elle est nommée « rendu_monteCarlo_simple » lorsque la méthode de Monte Carlo simple est utilisée et « rendu_monteCarlo_efficace » lorsque la méthode de Monte Carlo optimisée est utilisée. L’image est enregistrée au format HDR et peut être visualisée à l’aide de l’application tev.
