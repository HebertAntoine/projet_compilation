# projet_compilation

Ce projet est un mini-compilateur écrit avec **Lex/Flex** et **Yacc/Bison**.  
Il permet d’analyser un fichier `test.c`, de construire l’arbre syntaxique et de générer une image `arbre.png`.

---

## 🐳 Lancement avec Docker

Le projet est prévu pour être utilisé dans un conteneur Docker afin d’avoir directement tous les outils nécessaires (flex, bison, gcc, graphviz…).

### 1️⃣ Démarrer le conteneur

Depuis le dossier du projet sur ta machine :

```bash
docker run -it --rm -v $(pwd):/work -w /work gcc:latest bash
```

Puis, à l’intérieur du conteneur, installer les dépendances :

```bash
apt update
apt install -y flex bison make gcc graphviz
```

Tu te retrouves alors dans le dossier `/work` contenant le projet.

---

## 🔧 Compilation et génération de l’arbre

Pour compiler le projet et générer l’arbre syntaxique à partir de `test.c`, exécuter les commandes suivantes :

```bash
make realclean
make
./minicc test.c
ls -la apres_syntaxe.dot
head -n 10 apres_syntaxe.dot
dot -Tpng apres_syntaxe.dot -o arbre.png
ls -la arbre.png
```
---

## 🧹 Nettoyage du projet

Pour supprimer tous les fichiers générés par la compilation (fichiers .o, fichiers flex/bison, exécutable minicc, fichiers temporaires) :
```make clean```
Pour un nettoyage complet (y.tab.c, y.tab.h, lex.yy.c, minicc, out.s, etc.) :
```make realclean```
Cela permet de revenir à un projet propre avant une nouvelle compilation ou avant un commit Git.

---

## 📄 Résultat

- `apres_syntaxe.dot` : description de l’arbre syntaxique (format Graphviz)  
- `arbre.png` : image de l’arbre syntaxique générée à partir du fichier `.dot`

---

## 🧪 Exemple de fichier test

```c
void main() {
    int a = 120, b = 80;

    if (a > b) {
        a = a - b;
    }

    print("a = ", a, " - b = ", b);
}
```

