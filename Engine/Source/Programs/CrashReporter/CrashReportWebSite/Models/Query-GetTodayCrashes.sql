use crashreport
SELECT *
from crashes
where [TimeOfCrash] > CAST(GETDATE() AS DATE) 
--where [EpicAccountId] is not null and buildversion != '4.5.0.0' and buildversion != '4.5.1.0
--where buildversion like '4.5.1%' and [ChangeListVersion] != '0' and [PlatformName] not like 'Mac%' and [Branch] like 'UE4-Re%'



