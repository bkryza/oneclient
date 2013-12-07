%% ===================================================================
%% @author Rafal Slota
%% @copyright (C): 2013 ACK CYFRONET AGH
%% This software is released under the MIT license 
%% cited in 'LICENSE.txt'.
%% @end
%% ===================================================================
%% @doc: ConnectionSessionsTest cluster side test driver.      
%% @end
%% ===================================================================
-module(connection_sessions_test).
-include("test_common.hrl").

-export([setup/1, teardown/2, exec/1]).


setup(ccm) ->
    ok;
setup(worker) ->
    ok.


teardown(ccm, _State) ->
    ok; 
teardown(worker, _State) ->
    ok.


exec({env, VarName}) ->
    os:getenv(VarName);

%% Check if session with FuseID exists in DB
exec({check_session, FuseID}) ->
    case dao_lib:apply(dao_cluster, get_fuse_env, [FuseID], 1) of
        {ok, _}     -> ok;
        Other       -> Other
    end;
    
%% Check if varaibles are correctly placed in DB
exec({check_session_variables, FuseID, Vars}) ->
    case dao_lib:apply(dao_cluster, get_fuse_env, [FuseID], 1) of
        {ok, {veil_document, _, _, {fuse_env, _UID, _Hostname, Vars}, _}}     -> ok;
        Other  -> Other
    end.