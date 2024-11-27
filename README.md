# Groupe 9 - ESME

## **Auteurs**
- **Marwa Mostaghfir**
- **Kim Davy NDAYONGEJE**
- **Gaby Paultre**
- **Lou Allard**

---

## **Description du projet**
Ce projet consiste à adapter le module noyau Linux `mod4nb` pour le rendre compatible avec les versions de noyaux allant de 3.x à 6.x. Ce module fournit des tests relatifs aux fonctionnalités temporelles via le système de fichiers `/proc`, notamment les jiffies et les timers, en intégrant des API modernes pour maintenir la compatibilité avec les évolutions du noyau Linux.

---

## **Principaux changements**
- **Gestion des versions** :
  - Utilisation de `LINUX_VERSION_CODE` et `KERNEL_VERSION` pour détecter les différences d'API entre les versions de noyaux.
- **API modernisées** :
  - Remplacement de `PDE_DATA` par `proc_get_parent_data` pour les noyaux >= 5.0.
  - Remplacement de `do_gettimeofday` par `ktime_get_real_ns` pour les noyaux >= 5.0.
  - Substitution de `struct file_operations` par `struct proc_ops` pour les noyaux >= 5.6.
- **Synchronisation** :
  - Ajout d'un sémaphore pour les noyaux >= 3.3 afin de gérer les accès concurrents au buffer.
- **Robustesse accrue** :
  - Vérifications supplémentaires sur les appels système comme `copy_to_user` pour éviter les erreurs d'exécution.

---

## **Fonctionnalités**
- **/proc/HORclock** : Fournit les informations sur le temps courant et les jiffies.
- **/proc/HORbusy** : Simule une attente active (busy-wait).
- **/proc/HORsched** : Gère une attente planifiée en utilisant `schedule()`.
- **/proc/HORwait** : Implémente une attente avec interruption possible via `wait_event_interruptible_timeout`.
- **/proc/HORsched2** : Permet une attente préprogrammée via `schedule_timeout`.

---

## **Instructions pour tester le module**

### Compilation
1. Installez les headers du noyau correspondant à votre système.
2. Exécutez la commande suivante dans le répertoire contenant le fichier source `mod4nb.c` et le `Makefile` :
   ```bash
   make
   ```

### Chargement du module
Chargez le module dans le noyau :
```bash
sudo insmod mod4nb.ko
```

Vérifiez qu'il est bien chargé :
```bash
lsmod | grep mod4nb
```

### Utilisation
1. Listez les fichiers créés dans `/proc` :
   ```bash
   ls /proc | grep HOR
   ```
2. Consultez leur contenu pour tester les fonctionnalités :
   ```bash
   cat /proc/HORclock
   cat /proc/HORbusy
   ```

### Déchargement du module
Pour retirer le module une fois les tests terminés :
```bash
sudo rmmod mod4nb
```

Vérifiez qu'il est bien déchargé :
```bash
lsmod | grep mod4nb
```

### Analyse des logs
Pour consulter les messages système générés par le module, utilisez :
```bash
dmesg | tail -n 20
```

---

