
CONNECT ocihol/welcome;

-- STAGE 5
-- annotate table for result caching
-- Doing below part of stage5 in separate script so can see 
-- performance difference in AWR reports.
ALTER TABLE regions result_cache (mode force);
