<?php
    system("echo \"".$_POST["date"].",".$_POST["temp"].",".$_POST["humidity"]."\" >> temp.all.csv");
    system("tail -n2880 temp.all.csv > temp.csv");
?>
