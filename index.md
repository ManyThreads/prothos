---
layout: page
title: About ProThOS
---

![BMBF]({{site.baseurl}}/logo_bmbf.png){:class="img-logoright"}
![DLR]({{site.baseurl}}/logo_dlr.png){:class="img-logoright"}

Datenfluss- und Taskbasierte Modelle sind ungeachtet ihrer Potentiale noch verhältnismäßig neu im HPC Umfeld. Das Problem dabei ist, dass prinzipiell nur taskbasierte Programme ausgeführt werden, deren Workload-to-Communication Ratio entsprechend groß ist, d.h. die sehr grobgranular sind. Insbesondere die nötige Verwaltung der Ressourcen und der Datenverfügbarkeit generiert einen fundamentalen Overhead. Hinzu kommt, dass sich die Implementierung der Tasks an einer „typischen“ Hardware orientiert, während die Ausführung später auf heterogene oder anders strukturierte Hardware trifft.
ProThOS adressiert diese Herausforderungen, indem es die Programmierung und Ausführung von Tasks und Datenflüssen vom Programmiermodell bis hin zum Betriebssystem bearbeitet. Die Ausführungsgeschwindigkeit und Skalierbarkeit soll deutlich verbessert werden, indem alle relevanten Aspekte über die Ebenen hinweg aufeinander abgestimmt werden: Das bedeutet, dass 
* das Programmiermodell und die Compiler Abhängigkeits- und Granularitäts-Informationen exportieren ohne dabei die Programmierbarkeit zu beeinträchtigen, 
* die Ausführungsumgebung flexibler zwischen Abhängigkeiten auf Task- und datenparallelen Ebenen unterscheidet, 
* das Betriebssystem besser auf die Anforderungen dieser Modelle und Umgebungen ausgelegt ist. 

ProThOS zielt auf hocheffiziente Datenfluss- und task-basierte Anwendungen ab. ProThOS abstrahiert dabei nicht von der Hardware, sondern nutzt optimierte Ausführungs- und Codemuster, die auf verschiedene mit übertragen werden können. Um Overhead zu reduzieren, bezieht ProThOS das Betriebssystem mit ein, welches für task-basierte parallele Anwendungen ausgelegt wird. 
Die konkret angestrebten Ergebnisse von ProThOS sind dabei
* ein Nachweis darüber, dass feingranulare Tasks im HPC Bereich möglich und nutzbar sind
* ein Programmierkonzept für Arbeits- und Datenflüsse im HPC Nutzungsfeld, zusammen mit den Compiler & Tools, um hocheffizienten Code für verschiedene Platformen zu generieren
* eine portable Ausführungsumgebung die Abhängigkeiten und Lokalitäten berücksichtigt
* ein angepasstes Betriebssystem, das optimal auf Ressourcen- und Taskmanagement ausgelegt ist

Alle Ergebnisse werden im Rahmen von Referenzanwendungen verifiziert und evaluiert.


## Funding

The ProThOS project is funded by the Federal Ministry of Education and Research (BMBF) of Germany under Grant No. 01IH16011 from January 2017 to December 2019. The grant was part of the 5th HPC-Call of the Gauss Allianz.
