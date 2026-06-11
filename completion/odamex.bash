function __odamex {
  local -r IFS=$'\n'
  local -r cur="$2"
  local -r prev="$3"

  # options present on both client and server
  local -ar opts=(
    --version
    -iwad -file -deh -bex
    -map +map -wad -warp
    -config -port +logfile -confile
    -devparm -stepmode -blockmap -noflathack
    -nomonsters -respawn -fast -skill
    -waddir -cfgdir -crashdir
  )

  # client-only options
  local -ar clientopts=(
    -connect -numparticles
    -nomouse -nosound -nomusic -novideo -nodraw -noblit
    -netrecord -netplay
    -playdemo +playdemo
    +demotest -timedemo -shorttics
    -pistolstart -coop-things
    -fltk
  )

  # server-only options
  local -ar serveropts=(
    -timer -fork -avg -maxclients
  )

  # options that take a file as argument and associated extension
  local -Ar fileopts=(
    [-iwad]=.wad
    [-file]=.wad
    [-wad]=.wad
    [-deh]=.deh
    [-bex]=.bex
    [-config]=.cfg
    [+logfile]=.log
    [-netplay]=.odd
    [-playdemo]=.lmp
    [+playdemo]=.lmp
    [+demotest]=.lmp
    [-timedemo]=.lmp
    [-crashdir]=dir
    [-waddir]=dir
    [-cfgdir]=dir
    [-confile]=any
  )

  # options that don't make sense to use alongside others
  # for now this is only --version, but it's been designed
  # to be flexible in case we do anything like --help
  local -Ar exclusiveopts=(
    [--version]=1
  )

  # options that might get used more than once
  local -Ar repeatableopts=(
    [-file]=1
    [-deh]=1
    [-bex]=1
    [-waddir]=1
  )

  # options where we can't provide reasonable completion
  local -Ar nocompleteopts=(
    [-port]=1
    [-timer]=1
    [-maxclients]=1
    [-numparticles]=1
    [-skill]=1
    [-connect]=1
    [+map]=1
    [-map]=1
    [-warp]=1
    [-fltk]=1
  )

  # groups of options that don't make sense to use together
  local -Ar exclusivegroups=(
    [-timer]=timer
    [-avg]=timer
    [-netrecord]=demo
    [-netplay]=demo
    [-playdemo]=demo
    [+playdemo]=demo
    [+demotest]=demo
    [-timedemo]=demo
    [-map]=map
    [+map]=map
  )

  local -r cmd="$1"
  local exclusive_seen=false
  local -A used
  local -A used_groups
  local word
  for word in "${COMP_WORDS[@]:1}"; do
    [[ $word == "$cur" ]] && break
    [[ -v exclusiveopts[$word] ]] && exclusive_seen=true
    if [[ $word == [-+]* ]]; then
      used["$word"]=1
      local group="${exclusivegroups[$word]}"
      if [[ -n "$group" ]]; then
        used_groups["$group"]=1
      fi
    fi
  done

  if $exclusive_seen; then
    return
  fi

  if [[ -v nocompleteopts[$prev] ]]; then
    return
  fi

  # TODO: figure out if theres a good way to handle : in waddir
  if [[ -v fileopts[$prev] ]]; then
    compopt -o filenames

    local -r ext=${fileopts[$prev]}

    if [[ $ext == any ]]; then
      readarray -t COMPREPLY < <(compgen -f -- "$cur")
    elif [[ $ext == dir ]]; then
      readarray -t COMPREPLY < <(compgen -d -- "$cur")
    else
      compopt -o plusdirs
      readarray -t COMPREPLY < <(
        shopt -s extglob
        compgen -f -X "!*@(${ext@U}|${ext@L})" -- "$cur"
      )
    fi

    return
  fi

  local -a allopts=()
  case "$cmd" in
    odamex)
      allopts=("${opts[@]}" "${clientopts[@]}")
      ;;
    odasrv)
      allopts=("${opts[@]}" "${serveropts[@]}")
      ;;
  esac

  local -a available=()
  local opt
  for opt in "${allopts[@]}"; do
    [[ -v exclusiveopts[$opt] && ${#used[@]} -gt 0 ]] && continue

    if [[ ! -v repeatableopts[$opt] && -v used[$opt] ]]; then
      continue
    fi

    local group="${exclusivegroups[$opt]}"
    if [[ -n "$group" && -v used_groups[$group] ]]; then
      continue
    fi

    available+=("$opt")
  done

  readarray -t COMPREPLY < <(compgen -W "${available[*]}" -- "$cur")
}

if command -v odamex > /dev/null 2>&1; then
  complete -F __odamex odamex
fi

if command -v odasrv > /dev/null 2>&1; then
  complete -F __odamex odasrv
fi
