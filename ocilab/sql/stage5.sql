
CONNECT ocihol/welcome;

-- STAGE 5
-- annotate table for result caching so queries on it can be cached on 
-- client result cache
-- Doing below part of stage5 in separate script so can see 
-- performance difference in AWR reports.
ALTER TABLE regions result_cache (mode force);

QUIT;

