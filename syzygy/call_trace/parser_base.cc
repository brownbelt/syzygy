// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Implementation of call-trace parsing.
//
// TODO(rogerm): Rename parser_base.cc|h to parser.cc|h.

#include "syzygy/call_trace/parser_base.h"

#include "base/logging.h"
#include "sawbuck/common/buffer_parser.h"
#include "syzygy/call_trace/parse_engine_etw.h"
# if 0
// TODO(rogerm): Re-enable once http://codereview.appspot.com/5205041/ lands.
#include "syzygy/call_trace/parse_engine_rpc.h"
# endif

namespace call_trace {
namespace parser {

Parser::Parser() : active_parse_engine_(NULL) {
}

Parser::~Parser() {
  ignore_result(Close());

  ParseEngineIter it = parse_engine_set_.begin();
  for (; it != parse_engine_set_.end(); ++it) {
    delete *it;
  }
  parse_engine_set_.clear();
}

bool Parser::Init(ParseEventHandler* event_handler) {
  DCHECK(event_handler != NULL);
  DCHECK(active_parse_engine_ == NULL);

  ParseEngine* engine = NULL;

# if 0
  // TODO(rogerm): Re-enable once http://codereview.appspot.com/5205041/ lands.
  // Create the RPC call-trace parse engine.
  LOG(INFO) << "Initializing RPC call-trace parse engine.";
  engine = new ParseEngineRpc;
  if (engine == NULL) {
    LOG(ERROR) << "Failed to initialize RPC call-trace parse engine.";
    return false;
  }
  parse_engine_set_.push_back(engine);
# endif

  // Create the ETW call-trace parse engine.
  LOG(INFO) << "Initializing ETW call-trace parse engine.";
  engine = new ParseEngineEtw;
  if (engine == NULL) {
    LOG(ERROR) << "Failed to initialize ETW call-trace parse engine.";
    return false;
  }
  parse_engine_set_.push_back(engine);

  // Setup the event handler for all of the engines.
  ParseEngineIter it = parse_engine_set_.begin();
  for (; it != parse_engine_set_.end(); ++it) {
    (*it)->set_event_handler(event_handler);
  }

  return true;
}

bool Parser::OpenTraceFile(const FilePath& trace_file_path) {
  DCHECK(!trace_file_path.empty());

  if (active_parse_engine_ == NULL && !SetActiveParseEngine(trace_file_path)) {
    return false;
  }

  DCHECK(active_parse_engine_ != NULL);
  return active_parse_engine_->OpenTraceFile(trace_file_path);
}

bool Parser::Consume() {
  DCHECK(active_parse_engine_ != NULL);
  return active_parse_engine_->ConsumeAllEvents();
}

bool Parser::Close() {
  bool result = true;
  if (active_parse_engine_ != NULL) {
    result = active_parse_engine_->CloseAllTraceFiles();
    active_parse_engine_ = NULL;
  }
  return result;
}

bool Parser::SetActiveParseEngine(const FilePath& trace_file_path) {
  DCHECK(!trace_file_path.empty());
  DCHECK(active_parse_engine_ == NULL);

  ParseEngineIter it = parse_engine_set_.begin();
  for (; it != parse_engine_set_.end(); ++it) {
    ParseEngine* engine = *it;
    if (engine->IsRecognizedTraceFile(trace_file_path)) {
      LOG(INFO) << "Using " << engine->name() << " to parse trace files.";
      active_parse_engine_ = engine;
      return true;
    }
  }

  LOG(ERROR) << "Failed to find a parse engine for \""
             << trace_file_path.value()
             << "\".";

  return false;
}

}  // namespace call_trace::parser
}  // namespace call_trace
