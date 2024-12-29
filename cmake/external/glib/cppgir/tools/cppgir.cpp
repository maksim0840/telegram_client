#include "common.hpp"
#include "fs.hpp"
#include "genbase.hpp"
#include "genns.hpp"
#include "repository.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <map>
#include <set>
#include <vector>

// thanks go to glib
#define GI_STRINGIFY(macro_or_string) GI_STRINGIFY_ARG(macro_or_string)
#define GI_STRINGIFY_ARG(contents) #contents

#ifndef DEFAULT_IGNORE_FILE
// embed ignore data
#include "ignore.hpp"
static std::string GI_DEFAULT_IGNORE;
#else
static const char *GI_DATA_IGNORE = "";
static std::string GI_DEFAULT_IGNORE{GI_STRINGIFY(DEFAULT_IGNORE_FILE)};
#endif

Log _loglevel = Log::WARNING;

class Generator
{
  GeneratorContext &ctx_;
  std::vector<std::string> girdirs_;
  // processes ns (ns: ns header)
  std::map<std::string, std::string> processed_;

public:
  Generator(GeneratorContext &_ctx, const std::vector<std::string> &girdirs)
      : ctx_(_ctx), girdirs_(girdirs)
  {}

  std::string find_in_dir(const fs::path p, const std::string &ns) const
  {
    std::string result;
    fs::error_code ec;
    // check if in this directory
    if (!fs::is_directory(p, ec))
      return "";

    auto f = p / (ns + GIR_SUFFIX);
    std::vector<fs::directory_entry> dirs;
    for (auto &&entry : fs::directory_iterator(p, ec)) {
      if (fs::is_directory(entry, ec)) {
        dirs.emplace_back(entry);
      } else if (f == entry) {
        // exact match
        result = f.native();
      } else {
        // non-version match
        auto ename = entry.path().filename().native();
        auto s = ns.size();
        if (ename.substr(0, s) == ns && ename.size() > s && ename[s] == '-')
          result = entry.path().native();
      }
    }
    // check dirs
    while (!dirs.empty() && result.empty()) {
      result = find_in_dir(dirs.back(), ns);
      dirs.pop_back();
    }
    return result;
  }

  std::string find(const std::string &ns) const
  {
    for (auto &&d : girdirs_) {
      auto res = find_in_dir(fs::path(d), ns);
      if (!res.empty())
        return res;
    }
    return "";
  }

  // gir might be:
  // + a GIR filename
  // + or a GIR namespace (ns-version)
  // + or a GIR namespace (no version appended)
  void generate(const std::string &gir, bool recurse)
  {
    fs::path f(gir);
    fs::error_code ec;
    auto path = fs::exists(f, ec) ? gir : find(gir);
    if (path.empty())
      throw std::runtime_error("could not find GIR for " + gir);
    // avoid reading if possible
    if (processed_.count(gir))
      return;
    auto genp = NamespaceGenerator::new_(ctx_, path);
    auto &gen = *genp;
    // normalize namespace
    auto &&ns = gen.get_ns();
    // prevent duplicate processing
    if (processed_.count(ns))
      return;
    // generate deps
    auto &&deps = gen.get_dependencies();
    std::vector<std::string> headers;
    if (recurse) {
      for (auto &&d : deps) {
        generate(d, recurse);
        // should be available now
        headers.emplace_back(processed_[d]);
      }
    }
    // now generate this one
    // also mark processed and retain ns header file
    processed_[ns] = gen.process_tree(headers);
  }
};

static std::string
wrap(const char *s)
{
  std::string res;
  if (s)
    res = s;
  return res;
}

static int
die(const std::string &desc, const std::string &msg = "")
{
  std::cout << msg << std::endl << std::endl;
  std::cout << desc << std::endl;
  return 1;
}

static void
addsplit(std::vector<std::string> &target, const std::string &src,
    const std::string &seps = ":")
{
  std::vector<std::string> tmp;
  boost::split(tmp, src, boost::is_any_of(seps));
  for (auto &&d : tmp) {
    if (d.size())
      target.emplace_back(d);
  }
}

namespace options
{
using OptionParseFunc = std::function<bool(const char *)>;

struct Option
{
  std::string arg;
  OptionParseFunc func;
};

Option
make_parser(bool *val)
{
  auto h = [val](const char *v) {
    // no arg for command-line option, set to true in that case
    *val = v ? atoi(v) : true;
    return true;
  };
  return {"", h};
}

Option
make_parser(int *val)
{
  auto h = [val](const char *nextarg) {
    try {
      *val = std::stoi(nextarg);
      return true;
    } catch (const std::exception &exc) {
      return false;
    }
  };
  return {"number", h};
}

Option
make_parser(std::string *val)
{
  auto h = [val](const char *nextarg) {
    *val = nextarg;
    return true;
  };
  return {"arg", h};
}

} // namespace options

int
main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  using namespace options;

  // env/options targets
  int debug_level{};
  std::string fpath_ignore;
  std::string fpath_suppress{};
  std::string fpath_gen_suppress;
  std::string output_dir;
  bool doclass{};
  bool dofullclass{};
  bool use_dl{};
  bool use_expected{};
  bool const_method{};
  bool dump_ignore{};
  std::string gir_path;

  std::string helpdesc;
  struct OptionData
  {
    std::string opt;
    std::string var;
    const char *desc;
    Option option{};
  };
  std::vector<OptionData> descs = {
      {"help", "", "produce help message"},
      {"debug", "GI_DEBUG", "debug level", make_parser(&debug_level)},
      {"ignore", "GI_IGNORE", "colon separated ignore files",
          make_parser(&fpath_ignore)},
      {"suppression", "GI_SUPPRESSION", "colon separated suppression files",
          make_parser(&fpath_suppress)},
      {"gen-suppression", "G_GEN_SUPPRESSION", "generate suppression file",
          make_parser(&fpath_gen_suppress)},
      {"output", "GI_OUTPUT", "output directory", make_parser(&output_dir)},
      {"gir-path", "GI_GIR_PATH", "colon separated GIR search path",
          make_parser(&gir_path)},
      {"class", "GI_CLASS", "generate class implementation",
          make_parser(&doclass)},
      {"class-full", "GI_CLASS_FULL", "generate fallback class methods",
          make_parser(&dofullclass)},
      {"dl", "GI_DL", "use dynamic dlopen/dlsym rather than static link",
          make_parser(&use_dl)},
      {"expected", "GI_EXPECTED", "use expected<> return rather than exception",
          make_parser(&use_expected)},
      {"const-method", "GI_CONST_METHOD", "generate const methods",
          make_parser(&const_method)},
  };

  // optionally dump embedded ignore
  if (*GI_DATA_IGNORE) {
    descs.push_back({"dump-ignore", "", "dump embedded ignore data",
        make_parser(&dump_ignore)});
  }

  std::map<std::string, OptionData *> descs_index;

  // first collect settings from environment
  for (auto &e : descs) {
    // build index
    descs_index[e.opt] = &e;
    // check env var
    if (!e.var.empty() && e.option.func) {
      if (auto v = getenv(e.var.c_str())) {
        e.option.func(v);
      }
    }
  }

  // non-option argument analogue
  auto gir_top = wrap(getenv("GI_GIR"));

  // into settings
  std::vector<std::string> girs, ignore_files, suppress_files;
  std::vector<std::string> girdirs;

  // collect ignore files
  auto fpath_ignore_env = std::move(fpath_ignore);
  // suppress files
  auto fpath_suppress_env = std::move(fpath_suppress);
  // GIR path
  auto gir_path_env = std::move(gir_path);

  { // basic command line processing
    // assemble help description
    auto tmpl = (R"|(
{} [options] girs...

Supported options and environment variables
(specify 0 or 1 as variable value for a boolean switch):

)|");
    helpdesc = fmt::format(tmpl, argv[0]);
    for (auto &entry : std::as_const(descs)) {
      std::string var = entry.var;
      var = !var.empty() ? fmt::format("[{}] ", var) : var;
      helpdesc += fmt::format("  --{:<25}{}{}\n",
          entry.opt + ' ' + entry.option.arg, var, entry.desc);
    }
    if (!GI_DEFAULT_IGNORE.empty()) {
      helpdesc +=
          fmt::format("\nDefault ignore files:\n{}\n", GI_DEFAULT_IGNORE);
    }

    // simple command line processing
    for (int i = 1; i < argc; ++i) {
      std::string opt = argv[i];
      if (opt == "-h" || opt == "--help")
        return die(helpdesc);
      if (opt.size() > 2 && opt.find("--") == 0) {
        auto oi = descs_index.find(opt.substr(2));
        if (oi != descs_index.end()) {
          assert(oi->second);
          auto &option = *oi->second;
          assert(option.option.func);
          char *nextarg = nullptr;
          if (!option.option.arg.empty()) {
            if (i + 1 >= argc) {
              return die(helpdesc, opt + "; missing argument");
            } else {
              nextarg = argv[i + 1];
              ++i;
            }
          }
          logger(Log::LOG,
              fmt::format("processing option {} {}", opt, wrap(nextarg)));
          if (!option.option.func(nextarg))
            return die(helpdesc, opt + "; invalid argument " + nextarg);
        }
      } else if (!opt.empty() && opt[0] == '-') {
        return die(helpdesc, "unknown option " + opt);
      } else {
        girs.push_back(opt);
      }
    }
  }

  if (dump_ignore) {
    std::cout << GI_DATA_IGNORE << std::endl;
    return 0;
  }

  // collect some more files
  addsplit(ignore_files, fpath_ignore);
  addsplit(suppress_files, fpath_suppress);
  addsplit(girdirs, gir_path);

  // add env specified
  addsplit(ignore_files, fpath_ignore_env);
  addsplit(suppress_files, fpath_suppress_env);
  addsplit(girdirs, gir_path_env);

  // level
  if (debug_level > 0)
    _loglevel = (Log)debug_level;

  // system default
  {
    const char PATH_SEP = '/';
    // gobject-introspection considers XDG_DATA_HOME first
    auto xdg_data_home = wrap(getenv("XDG_DATA_HOME"));
    if (xdg_data_home.empty()) {
      xdg_data_home = wrap(getenv("HOME"));
      if (!xdg_data_home.empty()) {
        if (xdg_data_home.back() != PATH_SEP)
          xdg_data_home += PATH_SEP;
        xdg_data_home += fmt::format(".local{}share", PATH_SEP);
      }
    }
    std::vector<std::string> default_gir_dirs;
    if (!xdg_data_home.empty())
      default_gir_dirs.push_back(xdg_data_home);
    // gobject-introspection uses XDG_DATA_DIRS next
    auto xdg_data_dirs = wrap(getenv("XDG_DATA_DIRS"));
    addsplit(default_gir_dirs, xdg_data_dirs);
#ifdef DEFAULT_GIRPATH
    // optional fallback
    if (default_gir_dirs.empty())
      addsplit(default_gir_dirs, GI_STRINGIFY(DEFAULT_GIRPATH));
#endif
    for (auto &d : default_gir_dirs) {
      if (d.back() != PATH_SEP)
        d += PATH_SEP;
      d += "gir-1.0";
      girdirs.push_back(std::move(d));
    }
  }

  // extra GIR paths that gobject-introspection considers
#ifdef GI_GIR_DIR
  girdirs.push_back(GI_STRINGIFY(GI_GIR_DIR));
#endif
#ifdef GI_DATA_DIR
  girdirs.push_back(GI_STRINGIFY(GI_DATA_DIR));
#endif
#ifdef GI_DEF_DIR
  girdirs.push_back(GI_STRINGIFY(GI_DEF_DIR));
#endif

  for (auto &&d : girdirs)
    logger(Log::DEBUG, "extending GIR path " + d);

  // system default
  if (!GI_DEFAULT_IGNORE.empty())
    addsplit(ignore_files, GI_DEFAULT_IGNORE);

  // collect girs to process
  addsplit(girs, gir_top);

  // sanity check
  if (output_dir.empty())
    return die(helpdesc, "missing output directory");
  if (girs.empty())
    return die(helpdesc, "nothing to process");

  // check for now
  if (girdirs.empty())
    return die(helpdesc, "empty search path");

  // at least the standard ignore file is required
  // or things will go wrong
  int cnt = 0;
  fs::error_code ec;
  for (auto &f : ignore_files)
    cnt += fs::exists(f, ec);

  if (cnt == 0 && !GI_DEFAULT_IGNORE.empty())
    return die(helpdesc, "required default ignore file location not specified");

  auto match_ignore = Matcher(ignore_files, GI_DATA_IGNORE);
  auto match_suppress = Matcher(suppress_files);

  // now let's start
  GeneratorOptions options;
  options.rootdir = output_dir;
  options.classimpl = doclass;
  options.classfull = dofullclass;
  options.dl = use_dl;
  options.expected = use_expected;
  options.const_method = const_method;

  logger(Log::INFO, "generating to directory {}", options.rootdir);

  auto repo = Repository::new_();
  std::set<std::string> suppressions;
  GeneratorContext ctx{
      options, *repo, match_ignore, match_suppress, suppressions};
  Generator gen(ctx, girdirs);
  try {
    for (auto &&g : girs)
      gen.generate(g, true);

    // write suppression
    if (fpath_gen_suppress.size()) {
      std::vector<std::string> sup(suppressions.begin(), suppressions.end());
      sort(sup.begin(), sup.end());
      logger(Log::INFO, "writing {} suppressions to {}", sup.size(),
          fpath_gen_suppress);

      std::ofstream fsup(fpath_gen_suppress);
      for (auto &&v : sup)
        fsup << v << std::endl;
    }
  } catch (std::runtime_error &ex) {
    logger(Log::ERROR, ex.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
