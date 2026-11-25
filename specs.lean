import Std

namespace DocFind

abbrev Domain := String
abbrev Query := String
abbrev DocId := Nat
abbrev LineNum := Nat

structure DocEntry where
  id : DocId
  domain : Domain
  query : Query
  body : String
  headings : List LineNum
  deriving Repr, DecidableEq

structure OpenedDoc where
  docId : DocId
  line : LineNum
  readOnly : Bool
  deriving Repr, DecidableEq

structure State where
  allowedSites : List Domain
  cacheDocs : List DocEntry
  lastOpened : Option OpenedDoc
  vimReadOnly : Bool
  deriving Repr, DecidableEq

def initState : State :=
  { allowedSites := []
    cacheDocs := []
    lastOpened := none
    vimReadOnly := true }

def addAllowedSite (s : State) (d : Domain) : State :=
  let sites := if s.allowedSites.contains d then s.allowedSites else s.allowedSites ++ [d]
  { s with allowedSites := sites }

def openFromCache (s : State) (dId : DocId) (ln : LineNum) : State :=
  match s.cacheDocs.find? (fun doc => doc.id == dId) with
  | some _ =>
      { s with
        lastOpened := some { docId := dId, line := ln, readOnly := true }
        vimReadOnly := true }
  | none => s

def fetchAndCache (s : State) (q : Query) (d : Domain) (newDoc : DocEntry) : State :=
  if s.allowedSites.contains d && newDoc.domain == d && newDoc.query == q then
    { s with
      cacheDocs := s.cacheDocs ++ [newDoc]
      lastOpened := some { docId := newDoc.id, line := 1, readOnly := true }
      vimReadOnly := true }
  else
    s

inductive Step : State → State → Prop where
  | addSite (s : State) (d : Domain) :
      Step s (addAllowedSite s d)
  | openCache (s : State) (dId : DocId) (ln : LineNum) :
      Step s (openFromCache s dId ln)
  | fetchSite (s : State) (q : Query) (d : Domain) (newDoc : DocEntry) :
      Step s (fetchAndCache s q d newDoc)

def VimAlwaysReadOnly (s : State) : Prop :=
  s.vimReadOnly = true ∧
    match s.lastOpened with
    | some o => o.readOnly = true
    | none => True

def CacheRespectsAllowlist (s : State) : Prop :=
  ∀ doc ∈ s.cacheDocs, s.allowedSites.contains doc.domain = true

theorem step_preserves_vim_readonly {s s' : State}
    (hInv : VimAlwaysReadOnly s) (hStep : Step s s') :
    VimAlwaysReadOnly s' := by
  cases hStep with
  | addSite =>
      rcases hInv with ⟨h1, h2⟩
      exact ⟨h1, h2⟩
  | openCache =>
      rcases hInv with ⟨h1, _⟩
      simp [openFromCache, VimAlwaysReadOnly, h1]
  | fetchSite s q d newDoc =>
      rcases hInv with ⟨h1, _⟩
      by_cases hCond : s.allowedSites.contains d && newDoc.domain == d && newDoc.query == q
      · simp [fetchAndCache, hCond, VimAlwaysReadOnly]
      · simp [fetchAndCache, hCond, VimAlwaysReadOnly, h1, hInv.2]

theorem step_preserves_cache_allowlist {s s' : State}
    (hInv : CacheRespectsAllowlist s) (hStep : Step s s') :
    CacheRespectsAllowlist s' := by
  cases hStep with
  | addSite s d =>
      intro doc hdoc
      simp only [addAllowedSite] at hdoc ⊢
      split
      · exact hInv doc hdoc
      · rw [List.contains_append, hInv doc hdoc]
        rfl
  | openCache s dId ln =>
      exact hInv
  | fetchSite s q d newDoc =>
      intro doc hdoc
      by_cases hCond : s.allowedSites.contains d && newDoc.domain == d && newDoc.query == q
      · rw [fetchAndCache, if_pos hCond] at hdoc ⊢
        have h_cond := hCond
        simp only [Bool.and_eq_true, beq_iff_eq] at h_cond
        obtain ⟨⟨h_d_in, h_eq⟩, _⟩ := h_cond
        simp only [List.mem_append, List.mem_singleton] at hdoc
        rcases hdoc with hOld | rfl
        · exact hInv doc hOld
        · rw [h_eq]; exact h_d_in
      · rw [fetchAndCache, if_neg hCond]
        exact hInv doc hdoc

end DocFind
